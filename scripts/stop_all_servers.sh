#!/bin/bash
echo "Stopping MMORPG Server System..."

# PID 파일에서 프로세스 중지
if [ -f "logs/auth_server.pid" ]; then
    kill $(cat logs/auth_server.pid) 2>/dev/null && echo "Auth Server stopped"
    rm logs/auth_server.pid
fi

if [ -f "logs/gateway_server.pid" ]; then
    kill $(cat logs/gateway_server.pid) 2>/dev/null && echo "Gateway Server stopped"
    rm logs/gateway_server.pid
fi

if [ -f "logs/game_server.pid" ]; then
    kill $(cat logs/game_server.pid) 2>/dev/null && echo "Game Server stopped"
    rm logs/game_server.pid
fi

if [ -f "logs/zone_server.pid" ]; then
    kill $(cat logs/zone_server.pid) 2>/dev/null && echo "Zone Server stopped"
    rm logs/zone_server.pid
fi

echo "All servers stopped"
