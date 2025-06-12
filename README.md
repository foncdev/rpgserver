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

## 빠른 시작
```
bash# 1. 독립 설정 시스템 초기화
./scripts/setup_independent_configs.sh

# 2. 설정 검증
./scripts/validate_configs.sh development

# 3. 개발 환경 배포
./scripts/deploy.sh development

# 4. 서버 상태 모니터링
./scripts/monitor.sh development
```

## 프로덕션 배포
```
# 1. 환경변수 설정
export DB_AUTH_HOST=prod-db.example.com
export JWT_SECRET=super-secure-jwt-key

# 2. 프로덕션 설정 생성
./scripts/generate_production_configs.sh

# 3. Kubernetes 배포
DEPLOY_METHOD=k8s ./scripts/deploy.sh production
```

## 개별 서버 실행
```
# Auth Server만 실행
./dist/linux/bin/AuthServer

# 설정 파일 위치는 자동으로 config/auth_server.conf를 찾음
```