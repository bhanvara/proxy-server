# Epoll-based Proxy Server with Load Balancing

## Overview
This project implements an epoll-based proxy server in C with load balancing across multiple backend servers. The proxy efficiently handles concurrent client connections using an event-driven epoll loop (instead of a thread-per-connection approach). The dummy backend servers simulate real server processing and log their events. A simulation client is provided for testing the system.

## Features
- **Load Balancing (Least Connections):** Routes requests to the backend with the fewest active connections.
- **Epoll-based Event Loop:** Uses epoll for efficient handling of thousands of simultaneous connections on Linux.
- **Configurable Backend Servers:** Backend server addresses and ports (defined in `backend_servers.c`) are easy to adjust.
- **Logging:** Dummy backend servers log their output to files in the `backend_logs` directory.
- **Simulation Client:** A client simulator to generate requests for testing the proxy server.

## Build Instructions
To build the components, use the following commands:

- **Proxy Server:**  
    ```
    make -f Makefile.proxy
    ```
    
- **Dummy Backend Server:**  
    ```
    make -f Makefile.backend
    ```

- **Simulate Client:**  
    ```
    make -f Makefile.simclient
    ```

## Running the Components

### Proxy Server
Start the proxy server in a terminal:
```
./proxy_server
```
The proxy listens on port 8080 by default, accepts client connections, and forwards their requests to the least-loaded backend server.

### Dummy Backend Server
Run a dummy backend server on a specific port by executing:
```
./dummy_server [port]
```
Replace `[port]` with the desired port number (e.g., `9090`). You may start multiple backend servers manually or use the provided script.

**Starting All Backends:**

A script (`start_backends.sh`) is provided to start dummy backend servers on ports 9090–9099 at once. It creates a folder called `backend_logs` and directs each server’s output there:
```
./start_backends.sh
```

### Simulate Client
To test the setup, run the simulation client which issues multiple requests:
```
./simulate_client
```
Alternatively, you can use tools like `netcat`:
```
nc localhost 8080
```
Then type a message and press Enter to observe the response.

## Testing & Simulation

1. **Start Dummy Backend Servers:**  
   Either run each instance manually in separate terminals or use the script:
   ```
   ./start_backends.sh
   ```
   This launches backend servers on ports 9090 through 9099. Each logs output to respective files (e.g., `backend_logs/backend_9091.log`).

2. **Launch the Proxy Server:**  
   In another terminal, run:
   ```
   ./proxy_server
   ```

3. **Simulate Client Requests:**  
   Use the simulation client:
   ```
   ./simulate_client
   ```
   or manually send requests with netcat:
   ```
   nc localhost 8080
   ```

4. **Monitor Logs:**  
   Check the `backend_logs` directory for the backend servers' output. Each log file (for example, `backend_9091.log`) contains details on connections and processing events.

## Notes
- Ensure that all components (proxy, dummy backend, simulation client) are built before testing.
- Adjust backend server addresses and ports in `backend_servers.c` as needed.
- The proxy server now uses an epoll-based event loop, offering improved scalability over a multi-threaded design.
- All dummy backend server logs are automatically stored in the `backend_logs` folder.