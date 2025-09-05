#include "server.h"

// Structure to pass data to client handler thread
typedef struct {
    GameState* game;
    int player_index;
} ClientThreadData;

static GameState gamestate;
static int server_running = 1;

void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        fprintf(stderr, "\nShutting down server...\n");
        server_running = 0;
    }
}

void setup_signal_handlers() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE, SIG_IGN); // Ignore SIGPIPE
}

int create_server_socket(int port) {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Failed to create socket");
        return -1;
    }
    
    // Set socket options to reuse address
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(server_socket);
        return -1;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        return -1;
    }
    
    if (listen(server_socket, 10) < 0) {
        perror("Listen failed");
        close(server_socket);
        return -1;
    }
    
    return server_socket;
}

void cleanup_server() {
    // Close all client connections
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (gamestate.players[i].is_active) {
            close(gamestate.players[i].socket_fd);
        }
    }
    
    // Close server socket
    if (gamestate.server_socket != -1) {
        close(gamestate.server_socket);
    }
    
    cleanup_game_state(&gamestate);
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <port>\n", argv[0]);
    return 1;
  }

  int port = atoi(argv[1]);
  if (port <= 0 || port > 65535) {
    fprintf(stderr, "Invalid port number: %s\n", argv[1]);
    return 1;
  }

  // Setup signal handlers
  setup_signal_handlers();

  // Initialize game state
  init_game_state(&gamestate);

  // Create server socket
  gamestate.server_socket = create_server_socket(port);
  if (gamestate.server_socket == -1) {
    fprintf(stderr, "Failed to create server socket\n");
    return 1;
  }

  fprintf(stderr, "MOAB server started on port %d\n", port);

  // Main server loop
  while (server_running) {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    
    // Accept new client connection
    int client_socket = accept(gamestate.server_socket, 
                              (struct sockaddr*)&client_addr, &client_addr_len);
    
    if (client_socket == -1) {
      if (server_running) {
        perror("Accept failed");
      }
      continue;
    }
    
    // Add player to game
    int player_index = add_player(&gamestate, client_socket);
    if (player_index == -1) {
      fprintf(stderr, "No space available for new player\n");
      close(client_socket);
      continue;
    }

    fprintf(stderr, "New client connected, assigned to slot %d\n", player_index);
    
    // Create thread data structure
    ClientThreadData* thread_data = malloc(sizeof(ClientThreadData));
    if (!thread_data) {
      fprintf(stderr, "Failed to allocate thread data\n");
      remove_player(&gamestate, client_socket);
      continue;
    }
    
    thread_data->game = &gamestate;
    thread_data->player_index = player_index;
    
    // Create thread for client
    pthread_t thread;
    if (pthread_create(&thread, NULL, handle_client, thread_data) != 0) {
      fprintf(stderr, "Failed to create client thread\n");
      free(thread_data);
      remove_player(&gamestate, client_socket);
      continue;
    }
    
    // Detach thread (we don't need to join it)
    pthread_detach(thread);
  }
  
  // Cleanup
  cleanup_server();
  fprintf(stderr, "Server shutdown complete\n");
  
  return 0;
} 