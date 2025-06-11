#!/bin/bash
# =============================================================================
# scripts/build_and_setup.sh - 빌드 및 환경 설정 스크립트
# =============================================================================

echo "========================================="
echo "MMORPG Server System - Build & Setup"
echo "========================================="

# 스크립트 위치 확인
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "Project Root: $PROJECT_ROOT"
echo "Script Directory: $SCRIPT_DIR"

# 프로젝트 루트로 이동
cd "$PROJECT_ROOT"

# 운영체제 감지
OS=$(uname -s)
ARCH=$(uname -m)

echo "Detected OS: $OS"
echo "Detected Architecture: $ARCH"
echo ""

# 스크립트 실행 권한 부여
chmod +x scripts/*.sh

# 필요한 디렉토리 생성
echo "Creating necessary directories..."
mkdir -p config
mkdir -p logs
mkdir -p build
mkdir -p dist

# 설정 파일 생성 (없는 경우)
create_default_config() {
    echo "Creating default configuration files..."

    # 서버 설정 파일
    cat > config/server.conf << 'EOF'
# MMORPG Server Configuration File

[auth_server]
port = 8001
max_connections = 1000
log_level = INFO

[gateway_server]
port = 8002
max_connections = 5000
log_level = INFO

[game_server]
port = 8003
max_connections = 2000
tick_rate = 20
log_level = INFO

[zone_server]
port = 8004
max_connections = 1000
zone_id = 1
map_width = 50
map_height = 50
log_level = INFO

[network]
timeout = 30000
buffer_size = 8192
keep_alive = true

[logging]
console_output = true
file_output = true
filename = logs/mmorpg_server.log
level = INFO

[database]
host = localhost
port = 3306
name = mmorpg
user = mmorpg_user
password = password
EOF

    echo "✓ Default configuration created: config/server.conf"
}

# 시작 스크립트 생성
create_startup_scripts() {
    echo "Creating startup scripts..."

    # 모든 서버 시작 스크립트
    cat > scripts/start_all_servers.sh << 'EOF'
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
EOF

    # 모든 서버 중지 스크립트
    cat > scripts/stop_all_servers.sh << 'EOF'
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
EOF

    chmod +x scripts/start_all_servers.sh
    chmod +x scripts/stop_all_servers.sh

    echo "✓ Startup scripts created"
}

# README 파일 생성
create_readme() {
    cat > README.md << 'EOF'
# MMORPG Server System

A multi-server architecture MMORPG system with comprehensive logging and configuration management.

## Features

- **Multi-Server Architecture**: Auth, Gateway, Game, and Zone servers
- **Cross-Platform**: Linux, macOS (Intel & Apple Silicon), Windows
- **Comprehensive Logging**: File and console logging with configurable levels
- **Configuration Management**: INI-style configuration files with hot reload
- **High Performance**: 20 TPS game loop, optimized networking
- **Development Tools**: Test client with stress testing capabilities

## Quick Start

1. **Build the system:**
   ```bash
   ./scripts/build_and_setup.sh
   ```

2. **Start all servers:**
   ```bash
   ./scripts/start_all_servers.sh
   ```

3. **Test the system:**
   ```bash
   ./dist/*/bin/TestClient
   # Then use: connect localhost 8003
   ```

4. **Stop all servers:**
   ```bash
   ./scripts/stop_all_servers.sh
   ```

## Server Ports

- **Auth Server**: 8001
- **Gateway Server**: 8002
- **Game Server**: 8003
- **Zone Server**: 8004

## Configuration

Edit `config/server.conf` to customize server settings. Use the `reload` command in any server to apply changes without restart.

## Logging

- Console output: Enabled by default
- File output: `logs/` directory
- Log levels: DEBUG, INFO, WARNING, ERROR, CRITICAL

## Development

- Test client supports stress testing and multiple connection types
- All servers support runtime commands (status, reload, etc.)
- Modular architecture for easy expansion

## Architecture

```
Client -> Gateway Server -> Game Server -> Zone Server
              ^
              |
         Auth Server
```
EOF

    echo "✓ README.md created"
}

# 메인 빌드 함수
build_system() {
    echo "Building MMORPG Server System..."

    BUILD_SUCCESS=0
    BUILD_FAILED=0

    # 플랫폼별 빌드
    case "$OS" in
        "Linux")
            echo "Building for Linux..."
            if [ -f "scripts/build_linux.sh" ]; then
                ./scripts/build_linux.sh
                if [ $? -eq 0 ]; then
                    BUILD_SUCCESS=$((BUILD_SUCCESS + 1))
                    echo "✓ Linux build completed"
                else
                    BUILD_FAILED=$((BUILD_FAILED + 1))
                    echo "✗ Linux build failed"
                fi
            else
                echo "build_linux.sh not found!"
                BUILD_FAILED=$((BUILD_FAILED + 1))
            fi
            ;;
        "Darwin")
            if [ "$ARCH" = "arm64" ]; then
                echo "Building for macOS (Apple Silicon)..."
                if [ -f "scripts/build_macos_arm.sh" ]; then
                    ./scripts/build_macos_arm.sh
                    if [ $? -eq 0 ]; then
                        BUILD_SUCCESS=$((BUILD_SUCCESS + 1))
                        echo "✓ macOS (Apple Silicon) build completed"
                    else
                        BUILD_FAILED=$((BUILD_FAILED + 1))
                        echo "✗ macOS (Apple Silicon) build failed"
                    fi
                else
                    echo "build_macos_arm.sh not found!"
                    BUILD_FAILED=$((BUILD_FAILED + 1))
                fi
            else
                echo "Building for macOS (Intel)..."
                if [ -f "scripts/build_macos_intel.sh" ]; then
                    ./scripts/build_macos_intel.sh
                    if [ $? -eq 0 ]; then
                        BUILD_SUCCESS=$((BUILD_SUCCESS + 1))
                        echo "✓ macOS (Intel) build completed"
                    else
                        BUILD_FAILED=$((BUILD_FAILED + 1))
                        echo "✗ macOS (Intel) build failed"
                    fi
                else
                    echo "build_macos_intel.sh not found!"
                    BUILD_FAILED=$((BUILD_FAILED + 1))
                fi
            fi
            ;;
        "CYGWIN"*|"MINGW"*|"MSYS"*)
            echo "Windows environment detected (Cygwin/MinGW/MSYS)"
            echo "Please use build_all.bat for Windows builds"
            exit 1
            ;;
        *)
            echo "Unsupported OS: $OS"
            echo "Supported platforms: Linux, macOS"
            echo "For Windows, use build_all.bat"
            exit 1
            ;;
    esac

    return $BUILD_FAILED
}

# 설정 파일이 없으면 생성
if [ ! -f "config/server.conf" ]; then
    create_default_config
else
    echo "✓ Configuration file already exists"
fi

# 시작 스크립트 생성
create_startup_scripts

# README 파일 생성
create_readme

# 시스템 빌드
build_system
BUILD_RESULT=$?

echo ""
echo "========================================="
echo "Setup Summary:"

if [ $BUILD_RESULT -eq 0 ]; then
    echo "✓ Build completed successfully"
else
    echo "✗ Build failed"
fi

echo "✓ Configuration files created"
echo "✓ Startup scripts created"
echo "✓ Documentation created"
echo "✓ Directory structure initialized"
echo "========================================="

if [ $BUILD_RESULT -eq 0 ]; then
    echo ""
    echo "🎉 MMORPG Server System is ready!"
    echo ""
    echo "Next steps:"
    echo "1. Start all servers: ./scripts/start_all_servers.sh"
    echo "2. Test with client: ./dist/*/bin/TestClient"
    echo "3. Check logs: tail -f logs/*.log"
    echo "4. Customize config: edit config/server.conf"
    echo ""
    echo "For help, see README.md"
else
    echo ""
    echo "❌ Setup completed with build errors"
    echo "Please check the error messages above and fix any issues"
    echo "You may need to install missing dependencies"
fi