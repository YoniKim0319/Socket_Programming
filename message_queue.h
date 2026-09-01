#pragma once
#include <deque>
#include <optional>
#include "message.h"

class MessageQueue {
public:
    void Push(const Message& msg) {
        switch (msg.type) {
            case MessageType::HEARTBEAT:
                heartbeat_ = msg; // 최신 걸로 덮어씀 -> 오래된 heartbeat는 자동으로 버려짐
                break;
            case MessageType::TELEMETRY:
                telemetry_.push_back(msg);
                if (telemetry_.size() > kTelemetryCap) {
                    telemetry_.pop_front(); // 꽉 차면 제일 오래된 것부터 버림
                }
                break;
            case MessageType::EMERGENCY:
                emergency_.push_back(msg); // 절대 안 버림
                break;
        }
    }

    // 우선순위(EMERGENCY > TELEMETRY > HEARTBEAT) 순서로 하나 꺼냄.
    // 꺼낼 게 있으면 true를 반환하고 out에 채워줌, 없으면 false.
    bool Pop(Message& out) {
        if (!emergency_.empty()) {
            out = emergency_.front();
            emergency_.pop_front();
            return true;
        }
        if (!telemetry_.empty()) {
            out = telemetry_.front();
            telemetry_.pop_front();
            return true;
        }
        if (heartbeat_.has_value()) {
            out = *heartbeat_;
            heartbeat_.reset();
            return true;
        }
        return false;
    }

    bool Empty() const {
        return emergency_.empty() && telemetry_.empty() && !heartbeat_.has_value();
    }

private:
    std::optional<Message> heartbeat_; // 값이 있을 수도(최신 1개), 없을 수도 있음
    std::deque<Message> telemetry_;    // 양쪽 끝에서 넣고 뺄 수 있는 큐
    std::deque<Message> emergency_;

    static constexpr size_t kTelemetryCap = 100;
};
