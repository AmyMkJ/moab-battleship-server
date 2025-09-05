#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>

#define MAX_CLIENTS 100
#define MAX_NAME_LENGTH 21
#define BOARD_SIZE 10
#define SHIP_LENGTH 5
#define MAX_MESSAGE_LENGTH 100

// Ship structure
typedef struct {
    char owner[MAX_NAME_LENGTH];
    int x, y;  // center coordinates
    char direction;  // '-' for horizontal, '|' for vertical
    int damaged_cells[SHIP_LENGTH];  // 1 if cell is damaged, 0 otherwise
    int total_damage;  // count of damaged cells
} Ship;

// Player structure
typedef struct {
    int socket_fd;
    char name[MAX_NAME_LENGTH];
    Ship ship;
    int is_registered;
    int is_active;
} Player;

// Game state structure
typedef struct {
    Player players[MAX_CLIENTS];
    int player_count;
    pthread_mutex_t game_mutex;
    int server_socket;
} GameState;

// Function declarations
void init_game_state(GameState* game);
void cleanup_game_state(GameState* game);
int add_player(GameState* game, int socket_fd);
void remove_player(GameState* game, int socket_fd);
void broadcast_message(GameState* game, const char* message, int exclude_socket);
int handle_registration(GameState* game, int player_index, const char* message);
int handle_bomb(GameState* game, int player_index, const char* message);
int is_valid_ship_placement(int x, int y, char direction);
int is_name_taken(GameState* game, const char* name, int exclude_index);
void damage_ships_at_cell(GameState* game, int x, int y, const char* attacker);
void check_ship_destruction(GameState* game);
void* handle_client(void* arg);

// Server functions
void signal_handler(int sig);
void setup_signal_handlers(void);
int create_server_socket(int port);
void cleanup_server(void);

#endif // SERVER_H 