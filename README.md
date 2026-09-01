# Telematics Routing PoC

TCP 소켓 기반으로 우선순위가 다른 텔레매틱스 데이터(생존신호/텔레메트리/긴급알림)를 라우팅하고, 네트워크가 끊겨도 데이터를 잃지 않고 재전송하는 걸 검증하는 C++ 학습 프로젝트.

전체 배경, 검증 결과, 면접 Q&A까지 정리한 문서는 **[docs/PROJECT_SUMMARY.md](docs/PROJECT_SUMMARY.md)** 참고.

## 설계 근거

| 결정 | 왜 이렇게 했나 |
|---|---|
| 타입별 큐잉 정책이 다름 | HEARTBEAT(최신만 유지) / TELEMETRY(FIFO, 100개 cap) / EMERGENCY(무손실). 데이터 특성이 다르기 때문 — heartbeat는 상태 스냅샷이라 최신만 의미 있고, telemetry는 시계열이라 일부 손실 감내 가능하지만, emergency는 손실이 곧 사고 정보 유실이라 절대 버릴 수 없음 |
| 큐에서 우선순위 순서로 전송 (EMERGENCY 먼저) | 일반 데이터가 밀려있어도 긴급 데이터는 먼저 나가야 함 (head-of-line blocking 방지) |
| Exponential backoff 재연결 (1s→2s→4s→...→30s) | 서버 다운 중 고정 간격 재시도는 불필요한 부하를 계속 줌. 대기시간을 늘려가며 재시도해 부하를 줄임 |
| `send() + MSG_NOSIGNAL`로 연결 끊김 감지 | 끊긴 소켓에 `write()`하면 리눅스가 SIGPIPE로 프로세스를 강제종료시킴. `MSG_NOSIGNAL`로 대신 에러 리턴값을 받아 애플리케이션에서 재연결 처리 |
| 실패한 메시지는 버리지 않고 재연결 후 재전송 (`have_msg` 플래그) | 전송 중 끊겨도 그 메시지를 큐에서 이미 꺼낸 상태라 잃어버리기 쉬운데, 상태를 기억해뒀다가 재연결 후 같은 메시지를 다시 시도해 손실을 막음 |

**의도적으로 구현하지 않은 것** (시간 제약에 따른 스코프 조정, 자세한 이유는 PROJECT_SUMMARY.md 참고):
- EMERGENCY 전용 TCP 채널 분리 (지금은 단일 연결 + 큐 우선순위로 대체)
- 생성/전송 스레드 분리
- 실제 Wi-Fi on/off (서버 프로세스 강제종료/재시작으로 대체 — 클라이언트 입장에선 둘 다 소켓 쓰기 실패로 동일하게 감지됨)
- MQTT, CI/CT 자동화

## 파일 구조

```
telematics/
├── server.cpp          # 데이터 수신 서버 (타입별 로깅)
├── client.cpp           # 데이터 생성 → 큐잉 → 우선순위 전송 → 재연결
├── message.h             # Message 타입 정의 + 직렬화/역직렬화
├── message_queue.h       # 타입별 큐잉 정책 (MessageQueue 클래스)
└── docs/
    └── PROJECT_SUMMARY.md   # 전체 정리 (배경, 검증 결과, 면접 Q&A, 레쥬메 문구)
```

## 빌드 & 실행 (WSL, g++)

```bash
cd /mnt/d/telematics
g++ -o server server.cpp
g++ -o client client.cpp
```

터미널 1: `./server`
터미널 2: `./client`

연결 끊김 시나리오를 보려면, 클라이언트가 메시지를 보내는 도중(전송 사이 3초 대기가 있음) 터미널 1에서 서버를 Ctrl+C로 죽였다가 다시 `./server`로 재시작하면 됨 — exponential backoff로 재연결 후 남은 메시지를 이어서 전송하는 걸 확인할 수 있음.
