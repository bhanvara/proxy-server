#!/bin/bash
LOG_DIR="backend_logs"
mkdir -p "$LOG_DIR"

# List of all backend ports as defined in backend_servers.c
ports=(9090 9091 9092 9093 9094 9095 9096 9097 9098 9099)

for port in "${ports[@]}"
do
  echo "Starting backend server on port $port..."
  # Assuming the backend server executable is named dummy_server
  nohup ./dummy_server "$port" > "$LOG_DIR/backend_${port}.log" 2>&1 &
  sleep 0.5  # brief pause to avoid rapid forking issues
done

echo "All backend servers started. Logs are in the '$LOG_DIR' folder."