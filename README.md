# Epoll-based Proxy Server with Load Balancing and Caching

## Overview
This project implements an **epoll-based proxy server** in C with:
- **Load balancing** across multiple backend servers (using Least Connections strategy),
- An integrated **caching layer** for GET requests,
- **Efficient event-driven concurrency** using the `epoll` system call.

It simulates a scalable real-world proxy architecture, including:
- A **Proxy Server** that handles client connections, load balancing, and caching,
- Multiple **Dummy Backend Servers** that simulate real server behavior,
- A **Simulation Client** to generate and test load.

---

## Architecture Diagram

```
┌────────────┐            ┌─────────────────────────────────────┐            ┌─────────────────────┐
│  Clients    │──► request │        Epoll-based Proxy Server     │            │ Dummy Backend Server │
│ (SimClient, │───────────►│ + Load Balancer (Least Connections) │───────────►│   (on Port 9090+)    │
│   Netcat)   │            │ + In-Memory Caching (for GETs)       │            │     (or 9091, etc.)  │
└────────────┘            └─────────────────────────────────────┘            └─────────────────────┘
         ▲                                  │
         │                                  │
         └───────── cache HIT (for GET) ◄───┘
```

When a GET request is already cached, the proxy server immediately responds without contacting the backend.

---

## Features
- **Load Balancing (Least Connections):**  
  Distributes incoming client requests to the backend with the fewest active connections.

- **Epoll-based Event Loop:**  
  Provides highly scalable, non-blocking server architecture using `epoll`.

- **Caching Layer for GET Requests:**  
  - Frequently requested resources are cached in memory.
  - Reduces backend server load and improves response time for clients.
  - Cache entries automatically expire after a configurable **Time-to-Live (TTL)** (default: 60 seconds).

- **Configurable Backend Servers:**  
  Backend IP addresses and ports can be easily modified in `backend_servers.c`.

- **Logging:**  
  Dummy backend servers log their activity in the `backend_logs` directory.

- **Simulation Client:**  
  A utility to generate multiple client requests for testing the proxy server.

---

## Build Instructions

### Build All Components
Use the provided Makefiles to compile the components:

- **Proxy Server:**  
  ```
  make -f Makefile.proxy
  ```

- **Dummy Backend Server:**  
  ```
  make -f Makefile.backend
  ```

- **Simulation Client:**  
  ```
  make -f Makefile.simclient
  ```

Ensure that `cache.c` and `cache.h` are included when building the proxy server.

---

## Running the Components

### 1. Start Dummy Backend Servers
You can manually start individual backend servers:
```
./dummy_server [port]
```
Example:
```
./dummy_server 9090
```

Alternatively, launch multiple backend servers on ports 9090–9099 using the provided script:
```
./start_backends.sh
```
Logs will be created automatically under `backend_logs/`.

---

### 2. Start the Proxy Server
Launch the proxy server:
```
./proxy_server
```
The proxy listens on **port 8080** by default, forwards client requests to backend servers, and caches GET responses.

---

### 3. Run the Simulation Client
Generate simulated client requests:
```
./simulate_client
```
Alternatively, manually send requests using `netcat`:
```
nc localhost 8080
```
Example request:
```
GET /resource HTTP/1.1
```
(Press Enter twice after the request.)

---

## Testing and Verifying Caching Behavior

1. **Initial Request:**  
   The first `GET /resource` request is forwarded to a backend server and the response is cached.

2. **Subsequent Requests:**  
   Identical `GET /resource` requests are served directly from cache without contacting the backend.

3. **Observations:**  
   - Proxy server logs will show cache HIT or MISS events.
   - Backend server logs will show reduced repeated requests for cached resources.

4. **Cache Expiry:**  
   After the cache TTL (default 60 seconds), the cache entry expires automatically. A new request for the same resource will fetch from the backend again and recache the response.

---

## Notes
- Rebuild all components if any changes are made to the source code.
- Modify backend server addresses and ports easily in `backend_servers.c`.
- Cache TTL values can be adjusted in `cache.c`.
- All dummy backend server logs are stored under `backend_logs/`.

---

## Summary
| Feature | Description |
|:---|:---|
| Epoll Event Loop | Handles thousands of concurrent client connections efficiently. |
| Load Balancing | Distributes client load to the least-loaded backend server. |
| In-Memory Caching | Accelerates repeated GET responses and reduces backend server load. |
| Modular Design | Clean separation of proxy, backend server, and simulation client. |
| Scalable Testing | Simulation client enables high-load testing. |

---

## Quick Start

```bash
make -f Makefile.proxy
make -f Makefile.backend
make -f Makefile.simclient

./start_backends.sh
./proxy_server
./simulate_client
```
