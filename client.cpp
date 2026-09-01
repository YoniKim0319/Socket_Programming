#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "message.h"
#include "message_queue.h"

void EnqueueMessage(MessageQueue& queue, MessageType type, const std::string& payload) {
    Message msg;
    msg.type = type;
    msg.timestamp = 1700000000; // 임시 고정값, 나중에 실제 시각으로 교체
    msg.payload = payload;
    queue.Push(msg);
}

// 서버로 연결 1회 시도. 성공하면 소켓 fd, 실패하면 -1
int TryConnect() {
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        return -1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9000);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(sock_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(sock_fd);
        return -1;
    }
    return sock_fd;
}

// 연결될 때까지 재시도. 실패할 때마다 대기시간을 2배로 늘림 (최대 30초)
int ConnectWithBackoff() {
    int backoff_sec = 1;
    const int kMaxBackoff = 30;

    while (true) {
        int sock_fd = TryConnect();
        if (sock_fd >= 0) {
            std::cout << "Connected to server.\n";
            return sock_fd;
        }

        std::cout << "Connect failed. Retrying in " << backoff_sec << "s...\n";
        sleep(backoff_sec);

        backoff_sec *= 2;
        if (backoff_sec > kMaxBackoff) {
            backoff_sec = kMaxBackoff;
        }
    }
}

// 메시지 하나 전송 시도. 성공하면 true, 연결이 끊긴 상태면 false
bool SendMessage(int sock_fd, const Message& msg) {
    std::string line = ToLine(msg);
    // write() 대신 send()+MSG_NOSIGNAL: 끊긴 소켓에 쓰면 기본적으로
    // SIGPIPE 시그널이 발생해 프로세스가 그냥 죽어버림. MSG_NOSIGNAL은
    // 그 대신 send()가 그냥 -1을 리턴하게 해서, 우리가 직접 재연결 처리를 할 수 있게 함.
    ssize_t n = send(sock_fd, line.c_str(), line.size(), MSG_NOSIGNAL);
    if (n < 0) {
        return false;
    }
    std::cout << "Sent: " << line;
    return true;
}

int main() {
    MessageQueue queue;
    EnqueueMessage(queue, MessageType::HEARTBEAT, "alive");
    EnqueueMessage(queue, MessageType::TELEMETRY, "speed=80,lat=37.5,lon=127.0");
    EnqueueMessage(queue, MessageType::TELEMETRY, "speed=82,lat=37.51,lon=127.01");
    EnqueueMessage(queue, MessageType::EMERGENCY, "airbag_deployed");
    EnqueueMessage(queue, MessageType::HEARTBEAT, "alive"); // 이전 heartbeat를 덮어씀

    std::cout << "Queue built. Draining in priority order...\n";

    int sock_fd = ConnectWithBackoff();

    std::cout << "5 seconds before sending starts. Get the server terminal ready.\n";
    sleep(5);

    Message msg;
    bool have_msg = false;
    while (true) {
        if (!have_msg) {
            if (!queue.Pop(msg)) {
                break; // 큐가 비었으면 다 보낸 것
            }
            have_msg = true;
        }

        if (SendMessage(sock_fd, msg)) {
            have_msg = false; // 성공했으니 다음 메시지로 넘어감
            sleep(3);         // 테스트용 지연: 이 사이에 서버를 껐다 켜보기 위함
        } else {
            std::cout << "Send failed, connection lost. Reconnecting...\n";
            close(sock_fd);
            sock_fd = ConnectWithBackoff();
            // have_msg는 그대로 true라서, 재연결 후 같은 메시지를 다시 시도함
        }
    }

    close(sock_fd);
    return 0;
}
