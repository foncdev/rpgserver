#!/bin/bash

echo "Creating server configuration files..."

# 설정 디렉토리 생성
mkdir -p config
mkdir -p logs
mkdir -p maps
mkdir -p data

# Auth Server 설정
cat > config/auth_server.conf << 'EOF'
# Authentication Server Configuration

[server]
port = 8001
max_connections = 1000
log_level = INFO
log_file = logs/auth_server.log
console_output = true
file_output = true

[database]
host = localhost
port = 3306
name = mmorpg_auth
user = auth_user
password = auth_password
connection_pool_size = 10

[security]
jwt_secret = your-super-secret-jwt-key-change-this-in-production
jwt_expiration_hours = 24
password_hash_rounds = 12
ssl_enabled = false

EOF


# Gateway Server 설정
cat > config/gateway_server.conf << 'EOF'
# Gateway Server Configuration

[server]
port = 8002
max_connections = 5000
log_level = INFO
log_file = logs/gateway_server.log
console_output = true
file_output = true

[load_balance]
method = round_robin
health_check_interval = 30
connection_timeout = 5000
max_retries = 3
retry_delay = 1000

[upstream]
auth_servers = localhost:8001
game_servers = localhost:8003

[rate_limit]
enabled = true
requests = 100
window = 60
EOF

cat > config/game_server.conf << 'EOF'
# Game Server Configuration

[server]
port = 8003
max_connections = 2000
tick_rate = 20
log_level = INFO
log_file = logs/game_server.log
console_output = true
file_output = true

[game]
max_players_per_zone = 100
player_move_speed = 5.0
view_distance = 50
pvp_enabled = true
save_interval = 300

[performance]
worker_threads = 4
update_queue_size = 1000
optimized_networking = true
batch_size = 10

[zones]
servers = localhost:8004
connection_timeout = 5000
EOF


cat > config/zone_server.conf << 'EOF'
# Zone Server Configuration

[server]
port = 8004
max_connections = 1000
zone_id = 1
log_level = INFO
log_file = logs/zone_server.log
console_output = true
file_output = true

[map]
width = 100
height = 100
file = maps/zone_1.map
validation_enabled = true

[npc]
max_npcs = 200
spawn_interval = 5
data_file = data/npcs.json

[instance]
enabled = false
max_instances = 10
timeout = 3600

[physics]
tick_rate = 60.0
collision_enabled = true
gravity = 9.81
EOF