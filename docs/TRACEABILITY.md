# Requirements Traceability Matrix

| Requirement | Component | Verification |
|---|---|---|
| User login | ChatServer | Redis or MySQL validation |
| Message send/receive | ChatClient + ChatServer | TCP message exchange |
| Load balancing | LoadBalancer | Round-robin backend selection |
| Heartbeats | Client + Server | UDP heartbeat packets |
| Persistence | DBWriter + MySQL | Kafka -> MySQL inserts |
| Redis cache auth | ChatServer | cache hit / fallback |
| Multi-client | ChatServer | concurrent client threads |
| Graceful shutdown | Server + client | stop loops and cleanup |
