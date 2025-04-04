# Multi-threaded Proxy Server with Load Balancing

## Overview
This project implements a multi-threaded proxy server in C. It uses a thread pool to handle concurrent client connections efficiently and supports load balancing across multiple backend servers.

## Features
- **Load Balancing (Least Connections)**: The proxy routes requests to the backend server with the fewest active connections for balanced distribution.
- **Thread Pool**: Manages multiple client connections simultaneously.
- **Configurable Settings**: Allows customization of ports and backend server addresses.

## Build Instructions
To build the components of the project, use the following commands:

- **Proxy Server**:  
    ```
    make -f Makefile.proxy
    ```

- **Dummy Backend Server**:  
    ```
    make -f Makefile.backend
    ```

- **Simulate Client**:  
    ```
    make -f Makefile.simclient
    ```

## Running the Components

### Proxy Server
After building, start the proxy server with:  
```
./proxy_server
```

### Dummy Backend Server
Run the dummy backend server on a specific port:  
```
./dummy_server [port]
```
*Replace `[port]` with the desired port number (e.g., `9090`).*

### Simulate Client
Run the simulate client to test the proxy server:  
```
./simulate_client
```

## Testing & Simulation

To test and simulate the proxy server, follow these steps:

1. **Start Multiple Dummy Backend Servers**:  
     Launch backend servers on different ports in separate terminals:  
     - Terminal 1:  
       ```
       ./dummy_server 9090
       ```
     - Terminal 2:  
       ```
       ./dummy_server 9091
       ```

2. **Run the Proxy Server**:  
     Start the proxy server in another terminal:  
     ```
     ./proxy_server
     ```

3. **Simulate Client Requests**:  
     Use tools like `netcat` to send requests to the proxy server:  
     ```
     nc localhost 8080
     ```
     Type a message and press Enter to observe the response.

4. **Run the Simulate Client Program**:  
     Execute the simulate client program to generate multiple requests:  
     ```
     ./simulate_client
     ```

5. **Monitor Logs**:  
     Check the proxy server's terminal output for details on:  
     - Incoming client connections  
     - Backend server selection and load balancing  
     - Data relay between clients and backend servers  
     - Responses from backend servers

## Notes
- Ensure all components are built before running.
- Use different terminals for each server and client instance.
- Logs provide insights into the server's behavior and request handling.

