# Architecture Overview

This project implements a Windows C++ messaging system using:
- MFC desktop client
- Winsock2 TCP/UDP
- Redis cache for auth
- Kafka production and DBWriter consumption
- MySQL persistence
- round-robin TCP load balancer

## Topology

Client -> TCP 5000 -> Load Balancer -> Server1 / Server2

Server 1: TCP 6001 / UDP 7001
Server 2: TCP 6002 / UDP 7002

Each server validates users against Redis first, and falls back to MySQL if cache is unavailable.

## Flows

- Client connects to load balancer on port 5000
- Load balancer routes connections to a healthy backend
- Server authenticates user
- User sends chat message
- Server enqueues Kafka event
- DBWriter consumes Kafka and inserts into MySQL
- Redis Pub/Sub broadcasts to other servers

## Thread model

- UI thread in MFC client
- TCP receive thread
- UDP heartbeat thread
- server accept loop
- per-client worker thread
- UDP heartbeat monitor
- Kafka producer loop
- Kafka consumer loop
