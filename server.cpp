#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "message.h"

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "socket() failed\n";
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(9000);

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "bind() failed\n";
        return 1;
    }

    if (listen(server_fd, 1) < 0) {
        std::cerr << "listen() failed\n";
        return 1;
    }

    std::cout << "Server listening on port 9000...\n";

    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        std::cerr << "accept() failed\n";
        return 1;
    }

    std::cout << "Client connected.\n";

    std::string buffer;
    char chunk[1024];
    while (true) {
        ssize_t n = read(client_fd, chunk, sizeof(chunk));
        if (n <= 0) {
            std::cout << "Client disconnected.\n";
            break;
        }
        buffer.append(chunk, n);

        // TCP는 스트림이라 한 번의 read()에 메시지가 여러 개 붙어오거나
        // 잘려서 올 수 있음 -> '\n' 기준으로 완결된 줄만 꺼내서 처리
        size_t pos;
        while ((pos = buffer.find('\n')) != std::string::npos) {
            std::string line = buffer.substr(0, pos + 1);
            buffer.erase(0, pos + 1);

            Message msg = FromLine(line);
            std::cout << "[" << TypeToString(msg.type) << "] "
                      << "ts=" << msg.timestamp << " "
                      << "payload=" << msg.payload << "\n";
        }
    }

    close(client_fd);
    close(server_fd);
    return 0;
}
