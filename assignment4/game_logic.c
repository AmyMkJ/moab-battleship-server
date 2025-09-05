#include "server.h"

void init_game_state(GameState* game) {
    game->player_count = 0;
    game->server_socket = -1;
    pthread_mutex_init(&game->game_mutex, NULL);
    
    // Initialize all players as inactive
    for (int i = 0; i < MAX_CLIENTS; i++) {
        game->players[i].socket_fd = -1;
        game->players[i].is_registered = 0;
        game->players[i].is_active = 0;
        memset(game->players[i].name, 0, MAX_NAME_LENGTH);
        memset(&game->players[i].ship, 0, sizeof(Ship));
    }
}
void cleanup_game_state(GameState* game) {
    pthread_mutex_destroy(&game->game_mutex);
}

int add_player(GameState* game, int socket_fd) {
    pthread_mutex_lock(&game->game_mutex);
    
    // Find an empty slot
    int index = -1;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!game->players[i].is_active) {
            index = i;
            break;
        }
    }
    
    if (index == -1) {
        pthread_mutex_unlock(&game->game_mutex);
        return -1; // No space available
    }
    
    game->players[index].socket_fd = socket_fd;
    game->players[index].is_active = 1;
    game->players[index].is_registered = 0;
    game->player_count++;
    
    pthread_mutex_unlock(&game->game_mutex);
    return index;
}

void remove_player(GameState* game, int socket_fd) {
    pthread_mutex_lock(&game->game_mutex);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (game->players[i].socket_fd == socket_fd && game->players[i].is_active) {
            if (game->players[i].is_registered) {
                // 先广播消息（排除这个玩家）
                char gg_message[100];
                snprintf(gg_message, sizeof(gg_message), "GG %s\n", game->players[i].name);
                broadcast_message(game, gg_message, socket_fd);
                
                // 再设置状态
                game->players[i].is_active = 0;
                game->players[i].is_registered = 0;
                game->player_count--;
            } else {
                // 未注册的玩家直接清理
                game->players[i].is_active = 0;
                game->players[i].is_registered = 0;
                game->player_count--;
            }
            
            close(socket_fd);
            break;
        }
    }
    
    pthread_mutex_unlock(&game->game_mutex);
}

void broadcast_message(GameState* game, const char* message, int exclude_socket) {
    pthread_mutex_lock(&game->game_mutex);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (game->players[i].is_active && 
            game->players[i].is_registered && 
            game->players[i].socket_fd != exclude_socket) {
            
            if (send(game->players[i].socket_fd, message, strlen(message), 0) == -1) {
                // 发送失败，断开连接
                fprintf(stderr, "发送消息给玩家 %s 失败\n", game->players[i].name);
                close(game->players[i].socket_fd);
                game->players[i].is_active = 0;
                game->players[i].is_registered = 0;
                game->player_count--;
            }
        }
    }
    
    pthread_mutex_unlock(&game->game_mutex);
}

int is_valid_ship_placement(int x, int y, char direction) {
    if (direction == '-') {
        // Horizontal ship: check if all 5 cells are within bounds
        if (x < 2 || x > 7) return 0; // center must be at least 2 cells from edge
        if (y < 0 || y >= BOARD_SIZE) return 0;
    } else if (direction == '|') {
        // Vertical ship: check if all 5 cells are within bounds
        if (x < 0 || x >= BOARD_SIZE) return 0;
        if (y < 2 || y > 7) return 0; // center must be at least 2 cells from edge
    } else {
        return 0; // invalid direction
    }
    
    return 1;
}

int is_name_taken(GameState* game, const char* name, int exclude_index) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (i != exclude_index && game->players[i].is_active && 
            game->players[i].is_registered && 
            strcmp(game->players[i].name, name) == 0) {
            return 1;
        }
    }
    return 0;
}

int handle_registration(GameState* game, int player_index, const char* message) {
    char name[MAX_NAME_LENGTH];
    int x, y;
    char direction;
    
    // Parse REG message: "REG %20s %d %d %c\n"
    if (sscanf(message, "REG %20s %d %d %c", name, &x, &y, &direction) != 4) {
        return 0; // Invalid syntax
    }
    
    // Validate name (only letters, digits, or -)
    for (int i = 0; name[i] != '\0'; i++) {
        if (!((name[i] >= 'a' && name[i] <= 'z') || 
              (name[i] >= 'A' && name[i] <= 'Z') || 
              (name[i] >= '0' && name[i] <= '9') || 
              name[i] == '-')) {
            return 0; // Invalid character in name
        }
    }
    
    // Check if name is already taken
    if (is_name_taken(game, name, player_index)) {
        return 2; // Name taken
    }
    
    // Validate ship placement
    if (!is_valid_ship_placement(x, y, direction)) {
        return 0; // Invalid ship placement
    }
    
    // Registration successful
    pthread_mutex_lock(&game->game_mutex);
    
    strcpy(game->players[player_index].name, name);
    game->players[player_index].ship.x = x;
    game->players[player_index].ship.y = y;
    game->players[player_index].ship.direction = direction;
    strcpy(game->players[player_index].ship.owner, name);
    game->players[player_index].ship.total_damage = 0;
    
    // Initialize damaged cells array
    for (int i = 0; i < SHIP_LENGTH; i++) {
        game->players[player_index].ship.damaged_cells[i] = 0;
    }
    
    game->players[player_index].is_registered = 1;
    
    pthread_mutex_unlock(&game->game_mutex);
    
    return 1; // Success
}

int handle_bomb(GameState* game, int player_index, const char* message) {
    int x, y;
    
    // Parse BOMB message: "BOMB %d %d\n"
    if (sscanf(message, "BOMB %d %d", &x, &y) != 2) {
        return 0; // Invalid syntax
    }
    
    // Note: We don't validate bounds for bombing as per requirements
    // Let players bomb out of bounds if they want to miss
    
    damage_ships_at_cell(game, x, y, game->players[player_index].name);
    check_ship_destruction(game);
    
    return 1; // Success
}

void damage_ships_at_cell(GameState* game, int x, int y, const char* attacker) {
    pthread_mutex_lock(&game->game_mutex);
    
    int hit_any_ship = 0;
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!game->players[i].is_active || !game->players[i].is_registered) {
            continue;
        }
        
        Ship* ship = &game->players[i].ship;
        int ship_cells[SHIP_LENGTH][2]; // [x, y] coordinates of ship cells
        
        // Calculate ship cell coordinates
        if (ship->direction == '-') {
            // Horizontal ship
            for (int j = 0; j < SHIP_LENGTH; j++) {
                ship_cells[j][0] = ship->x - 2 + j; // x coordinates
                ship_cells[j][1] = ship->y; // y coordinate
            }
        } else {
            // Vertical ship
            for (int j = 0; j < SHIP_LENGTH; j++) {
                ship_cells[j][0] = ship->x; // x coordinate
                ship_cells[j][1] = ship->y - 2 + j; // y coordinates
            }
        }
        
        // Check if bomb hits this ship
        for (int j = 0; j < SHIP_LENGTH; j++) {
            if (ship_cells[j][0] == x && ship_cells[j][1] == y) {
                // Hit! Mark cell as damaged if not already
                if (!ship->damaged_cells[j]) {
                    ship->damaged_cells[j] = 1;
                    ship->total_damage++;
                    
                    // Broadcast HIT message
                    char hit_message[100];
                    snprintf(hit_message, sizeof(hit_message), "HIT %s %d %d %s\n", 
                             attacker, x, y, ship->owner);
                    broadcast_message(game, hit_message, -1);
                    hit_any_ship = 1;
                }
                break;
            }
        }
    }
    
    // If no ships were hit, broadcast MISS message
    if (!hit_any_ship) {
        char miss_message[100];
        snprintf(miss_message, sizeof(miss_message), "MISS %s %d %d\n", attacker, x, y);
        broadcast_message(game, miss_message, -1);
    }
    
    pthread_mutex_unlock(&game->game_mutex);
}

void check_ship_destruction(GameState* game) {
    pthread_mutex_lock(&game->game_mutex);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!game->players[i].is_active || !game->players[i].is_registered) {
            continue;
        }
        
        Ship* ship = &game->players[i].ship;
        
        // Check if ship is completely destroyed (all 5 cells damaged)
        if (ship->total_damage == SHIP_LENGTH) {
            // Ship destroyed! Player loses
            char gg_message[100];
            snprintf(gg_message, sizeof(gg_message), "GG %s\n", ship->owner);
            broadcast_message(game, gg_message, -1);
            
            // Disconnect the player
            close(game->players[i].socket_fd);
            game->players[i].is_active = 0;
            game->players[i].is_registered = 0;
            game->player_count--;
        }
    }
    
    pthread_mutex_unlock(&game->game_mutex);
} 