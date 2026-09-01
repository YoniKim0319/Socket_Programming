# Telematics Routing PoC

A small C++ learning project that demonstrates **priority-based routing of telematics-style messages over TCP sockets** and verifies that messages can be retained and retransmitted when the network connection is interrupted.

The client handles three types of data with different priorities and queueing policies:

* `HEARTBEAT` — vehicle/system liveness information
* `TELEMETRY` — periodic telemetry data
* `EMERGENCY` — high-priority emergency alerts

The main goal of this project is to practice **C++ socket programming, message queue management, priority-based transmission, and connection recovery** in a simplified telematics scenario.

## Design Decisions

| Decision                                                          | Rationale                                                                                                                                                                                                                                                                                                                                                                                              |
| ----------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Different queueing policies for each message type                 | Each type of data has different characteristics. `HEARTBEAT` keeps only the latest value because it represents the current state. `TELEMETRY` uses FIFO with a maximum queue size of 100 because older telemetry may become less useful when the queue grows too large. `EMERGENCY` messages are never intentionally dropped because losing an emergency event could mean losing critical information. |
| Priority-based transmission (`EMERGENCY` first)                   | Emergency messages should not be delayed by a backlog of normal telemetry data. The sender checks higher-priority queues first so critical messages can be transmitted as soon as the connection is available.                                                                                                                                                                                         |
| Exponential backoff for reconnection (`1s → 2s → 4s → ... → 30s`) | Retrying at a fixed short interval while the server is unavailable can generate unnecessary connection attempts. Exponential backoff gradually increases the retry interval and limits the maximum delay to 30 seconds.                                                                                                                                                                                |
| `send()` with `MSG_NOSIGNAL` to detect connection failures        | On Linux, writing to a closed socket may generate `SIGPIPE`, which can terminate the process. Using `MSG_NOSIGNAL` prevents this signal and allows the application to handle the error returned by `send()` instead.                                                                                                                                                                                   |
| Retain the current message after a failed transmission            | A message may already have been removed from the queue when the connection fails. The client keeps the current message using a `have_msg` state and retries the same message after reconnection instead of immediately discarding it.                                                                                                                                                                  |

## Project Structure

```text
telematics/
├── server.cpp          # TCP server and message-type logging
├── client.cpp          # Message generation, queueing, priority sending, and reconnection
├── message.h           # Message definitions and serialization/deserialization
└── message_queue.h     # Per-type queueing policies (MessageQueue class)
```

## Build

Tested on **WSL with g++**.

```bash
cd /mnt/d/telematics

g++ -o server server.cpp
g++ -o client client.cpp
```

## Run

Start the server:

```bash
./server
```

In another terminal, start the client:

```bash
./client
```

The client generates messages, stores them in type-specific queues, and sends them according to priority.

## Connection Failure Test

To test the reconnection behavior:

1. Start both the server and client.
2. While the client is sending messages, stop the server with `Ctrl+C`.
3. Observe the client detecting the connection failure.
4. The client attempts to reconnect using exponential backoff.
5. Restart the server:

```bash
./server
```

6. After reconnection, the client resumes transmission and retries the message that failed during the previous connection.

A 3-second delay between transmissions is intentionally included to make the behavior easier to observe.

## What I Practiced

Through this project, I practiced:

* TCP socket programming in C++
* Client/server communication
* Basic message serialization and deserialization
* Queue-based message management
* Priority-based data transmission
* Handling socket disconnections
* `SIGPIPE` handling with `MSG_NOSIGNAL`
* Exponential-backoff reconnection
* Retrying an in-flight message after connection recovery

This project is intentionally kept small and focuses on understanding the behavior of **TCP communication and application-level message handling** rather than implementing a production-grade telematics communication stack.
