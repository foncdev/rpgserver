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
