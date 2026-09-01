#pragma once
#include <string>
#include <cstdint>

enum class MessageType {
    HEARTBEAT,
    TELEMETRY,
    EMERGENCY
};

struct Message {
    MessageType type;
    uint64_t timestamp;   // 생성된 시각 (초 단위)
    std::string payload;  // 실제 내용, 지금은 단순 문자열로
};

inline const char* TypeToString(MessageType type) {
    switch (type) {
        case MessageType::HEARTBEAT: return "HEARTBEAT";
        case MessageType::TELEMETRY: return "TELEMETRY";
        case MessageType::EMERGENCY: return "EMERGENCY";
    }
    return "UNKNOWN";
}

// Message -> "타입|타임스탬프|내용\n" 형태의 한 줄 문자열로 변환 (전송용)
inline std::string ToLine(const Message& msg) {
    return std::to_string(static_cast<int>(msg.type)) + "|" +
           std::to_string(msg.timestamp) + "|" +
           msg.payload + "\n";
}

// 한 줄 문자열 -> Message로 복원 (수신용)
inline Message FromLine(const std::string& line) {
    size_t p1 = line.find('|');
    size_t p2 = line.find('|', p1 + 1);
    Message msg;
    msg.type = static_cast<MessageType>(std::stoi(line.substr(0, p1)));
    msg.timestamp = std::stoull(line.substr(p1 + 1, p2 - p1 - 1));
    msg.payload = line.substr(p2 + 1);
    if (!msg.payload.empty() && msg.payload.back() == '\n') {
        msg.payload.pop_back(); // ToLine에서 붙인 줄바꿈 문자 제거
    }
    return msg;
}
