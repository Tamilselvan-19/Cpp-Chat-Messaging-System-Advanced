# Cpp Chat Messaging System Advanced

This repository contains an advanced Windows C++ chat application built with:
- MFC desktop client
- Round-robin TCP load balancer
- Two independent chat servers
- Redis authentication cache with MySQL fallback
- Kafka message publication and DB writer consumption
- MySQL persistence
- UDP heartbeat monitoring

## Repository structure

- `ChatClient/` – MFC chat client
- `ChatServer/` – TCP server, Redis integration, Kafka producer
- `LoadBalancer/` – TCP proxy and health checks
- `DBWriter/` – Kafka consumer and SQL writer
- `Common/` – protocol, logging, JSON, INI, SHA-256 helpers
- `config/` – runtime configuration
- `database/` – SQL schema and sample data
- `docs/` – architecture and traceability notes
- `tests/` – test documentation and scenarios

## Quick start

1. Install Visual Studio 2022/2026 with C++ desktop workload.
2. Install dependencies using vcpkg.
3. Open `CppChatMessaging.sln`.
4. Build in `x64` release mode.
5. Start MySQL, Redis, and Kafka.
6. Run `DBWriter.exe`, both `ChatServer` instances, then `LoadBalancer.exe`.
7. Launch the client and connect to `127.0.0.1:5000`.

See `docs/ARCHITECTURE.md` for details.
