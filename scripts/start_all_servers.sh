#!/bin/bash
echo "Starting MMORPG Server System..."

# 플랫폼별 바이너리 경로 설정
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    BIN_PATH="dist/linux/bin"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    if [[ $(uname -m) == "arm64" ]]; then
        BIN_PATH="dist/macos_arm/bin"
    else
        BIN_PATH="dist/macos_intel/bin"
    fi
else
    echo "Unsupported platform"
    exit 1
fi

# 바이너리 존재 확인
if [ ! -d "$BIN_PATH" ]; then
    echo "Error: Binaries not found in $BIN_PATH"
    echo "Please run build script first"
    exit 1
fi

# 서버들을 백그라운드에서 시작
echo "Starting Auth Server..."
./$BIN_PATH/AuthServer &
AUTH_PID=$!

sleep 2

echo "Starting Gateway Server..."
./$BIN_PATH/GatewayServer &
GATEWAY_PID=$!

sleep 2

echo "Starting Game Server..."
./$BIN_PATH/GameServer &
GAME_PID=$!

sleep 2

echo "Starting Zone Server..."
./$BIN_PATH/ZoneServer &
ZONE_PID=$!

echo ""
echo "All servers started!"
echo "PIDs: Auth=$AUTH_PID, Gateway=$GATEWAY_PID, Game=$GAME_PID, Zone=$ZONE_PID"
echo ""
echo "To stop all servers, run: scripts/stop_all_servers.sh"
echo "To view logs: tail -f logs/*.log"

# PID 파일 저장
echo "$AUTH_PID" > logs/auth_server.pid
echo "$GATEWAY_PID" > logs/gateway_server.pid
echo "$GAME_PID" > logs/game_server.pid
echo "$ZONE_PID" > logs/zone_server.pid
