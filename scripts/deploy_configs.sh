#!/bin/bash

echo "Deploying server configurations for production..."

# 환경 변수 확인
ENVIRONMENT=${ENVIRONMENT:-development}
SERVER_TYPE=${SERVER_TYPE:-all}

echo "Environment: $ENVIRONMENT"
echo "Server Type: $SERVER_TYPE"

# 환경별 설정 디렉토리
ENV_CONFIG_DIR="config/${ENVIRONMENT}"
mkdir -p "$ENV_CONFIG_DIR"

# 프로덕션 환경 설정 생성
if [ "$ENVIRONMENT" = "production" ]; then
    echo "Creating production configurations..."

    # Auth Server 프로덕션 설정
    if [ "$SERVER_TYPE" = "all" ] || [ "$SERVER_TYPE" = "auth" ]; then
        cat > "$ENV_CONFIG_DIR/auth_server.conf" << 'EOF'
[server]
port = 8001
max_connections = 5000
log_level = WARNING
log_file = /var/log/mmorpg/auth_server.log
console_output = false
file_output = true

[database]
host = ${DB_AUTH_HOST}
port = ${DB_AUTH_PORT}
name = ${DB_AUTH_NAME}
user = ${DB_AUTH_USER}
password = ${DB_AUTH_PASSWORD}
connection_pool_size = 50

[security]
jwt_secret = ${JWT_SECRET}
jwt_expiration_hours = 8
password_hash_rounds = 15
ssl_enabled = true
EOF
        echo "✓ Production Auth Server config created"
    fi

    # Gateway Server 프로덕션 설정
    if [ "$SERVER_TYPE" = "all" ] || [ "$SERVER_TYPE" = "gateway" ]; then
        cat > "$ENV_CONFIG_DIR/gateway_server.conf" << 'EOF'
[server]
port = 8002
max_connections = 10000
log_level = WARNING
log_file = /var/log/mmorpg/gateway_server.log
console_output = false
file_output = true

[load_balance]
method = least_connections
health_check_interval = 15
connection_timeout = 3000
max_retries = 5
retry_delay = 500

[upstream]
auth_servers = ${AUTH_SERVERS}
game_servers = ${GAME_SERVERS}

[rate_limit]
enabled = true
requests = 1000
window = 60
EOF
        echo "✓ Production Gateway Server config created"
    fi

    # Game Server 프로덕션 설정
    if [ "$SERVER_TYPE" = "all" ] || [ "$SERVER_TYPE" = "game" ]; then
        cat > "$ENV_CONFIG_DIR/game_server.conf" << 'EOF'
[server]
port = 8003
max_connections = 5000
tick_rate = 30
log_level = WARNING
log_file = /var/log/mmorpg/game_server.log
console_output = false
file_output = true

[game]
max_players_per_zone = 200
player_move_speed = 5.0
view_distance = 75
pvp_enabled = true
save_interval = 180

[performance]
worker_threads = 8
update_queue_size = 5000
optimized_networking = true
batch_size = 50

[zones]
servers = ${ZONE_SERVERS}
connection_timeout = 3000
EOF
        echo "✓ Production Game Server config created"
    fi

    # Zone Server 프로덕션 설정
    if [ "$SERVER_TYPE" = "all" ] || [ "$SERVER_TYPE" = "zone" ]; then
        cat > "$ENV_CONFIG_DIR/zone_server.conf" << 'EOF'
[server]
port = 8004
max_connections = 2000
zone_id = ${ZONE_ID}
log_level = WARNING
log_file = /var/log/mmorpg/zone_server.log
console_output = false
file_output = true

[map]
width = 500
height = 500
file = /opt/mmorpg/maps/zone_${ZONE_ID}.map
validation_enabled = true

[npc]
max_npcs = 1000
spawn_interval = 3
data_file = /opt/mmorpg/data/npcs.json

[instance]
enabled = true
max_instances = 50
timeout = 7200

[physics]
tick_rate = 60.0
collision_enabled = true
gravity = 9.81
EOF
        echo "✓ Production Zone Server config created"
    fi
fi

echo "Configuration deployment completed!"