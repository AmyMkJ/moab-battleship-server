#include "server.h"

// Structure to pass data to client handler thread
typedef struct {
    GameState* game;
    int player_index;
} ClientThreadData;

void* handle_client(void* arg) {
    ClientThreadData* data = (ClientThreadData*)arg;
    GameState* game = data->game;
    int player_index = data->player_index;
    int socket_fd = game->players[player_index].socket_fd;
    
    free(data); // Free the thread data structure
    
    char buffer[MAX_MESSAGE_LENGTH + 1];
    int bytes_read;
    int message_length = 0;
    
    while (1) {
        // Read one character at a time to handle message boundaries
        bytes_read = recv(socket_fd, buffer + message_length, 1, 0);
        
        if (bytes_read <= 0) {
            // Client disconnected or error
            fprintf(stderr, "Client disconnected\n");
            remove_player(game, socket_fd);
            break;
        }
        
        message_length += bytes_read;
        
        // Check for newline (message end)
        if (buffer[message_length - 1] == '\n') {
            buffer[message_length] = '\0'; // Null terminate
            
            // Check message length limit
            if (message_length > MAX_MESSAGE_LENGTH) {
                fprintf(stderr, "Message too long, disconnecting client\n");
                remove_player(game, socket_fd);
                break;
            }
            
            // Process the message
            int result = 0;
            
            if (strncmp(buffer, "REG ", 4) == 0) {
                if (!game->players[player_index].is_registered) {
                    result = handle_registration(game, player_index, buffer);
                    
                    if (result == 1) {
                        // Registration successful
                        send(socket_fd, "WELCOME\n", 8, 0);
                        
                        // Broadcast JOIN message
                        char join_message[100];
                        snprintf(join_message, sizeof(join_message), "JOIN %s\n", 
                                 game->players[player_index].name);
                        broadcast_message(game, join_message, -1);
                    } else if (result == 2) {
                        // Name taken
                        send(socket_fd, "TAKEN\n", 6, 0);
                    } else {
                        // Invalid syntax or placement
                        send(socket_fd, "INVALID\n", 8, 0);
                    }
                } else {
                    // Already registered
                    send(socket_fd, "INVALID\n", 8, 0);
                }
            } else if (strncmp(buffer, "BOMB ", 5) == 0) {
                if (game->players[player_index].is_registered) {
                    result = handle_bomb(game, player_index, buffer);
                    
                    if (result == 0) {
                        // 只有语法错误才返回INVALID
                        send(socket_fd, "INVALID\n", 8, 0);
                    }
                    // 成功轰炸不返回消息
                } else {
                    // 未注册玩家发送BOMB - 忽略，不响应
                }
            } else {
                // 未知命令
                send(socket_fd, "INVALID\n", 8, 0);
            }
            
            // Reset for next message
            message_length = 0;
        }
        
        // Check if we've exceeded the message length limit
        if (message_length >= MAX_MESSAGE_LENGTH) {
            fprintf(stderr, "Message too long without newline, disconnecting client\n");
            remove_player(game, socket_fd);
            break;
        }
    }
    
    return NULL;
} 