# Multiplayer Online Asynchronous Battleship (MOAB) Server

A multi-threaded C server implementation for a real-time multiplayer battleship game, built as part of CSCB09 - Software Tools and Systems Programming course.

## 🎯 Project Overview

This project implements a TCP-based game server that supports multiple concurrent players in an asynchronous battleship game. Players can join, register, place ships, and attack each other in real-time.

## ✨ Key Features

- **Multi-threaded Architecture**: Each client connection runs in its own thread
- **Concurrent Player Support**: Supports up to 100 simultaneous players
- **Real-time Communication**: TCP-based message protocol for game actions
- **Ship Management**: 5-cell ships with horizontal/vertical placement
- **Damage System**: Track ship damage and destruction
- **Error Handling**: Robust error handling and resource management
- **Signal Handling**: Graceful server shutdown on SIGINT/SIGTERM

## 🏗️ Architecture

### Core Components

- **`server.c`**: Main server program with connection handling
- **`game_logic.c`**: Game state management and logic
- **`client_handler.c`**: Individual client message processing
- **`server.h`**: Shared data structures and function declarations

### Thread Model

```
Main Thread (server.c)
├── Accepts new connections
├── Creates client threads
└── Manages server lifecycle

Client Threads (client_handler.c)
├── Handles individual client messages
├── Processes game commands (REG, BOMB)
└── Manages client-specific state
```

## 🚀 Getting Started

### Prerequisites

- GCC compiler
- POSIX-compliant system (Linux/macOS)
- pthread library

### Compilation

```bash
make clean
make
```

### Running the Server

```bash
./moab_server <port>
```

Example:
```bash
./moab_server 8080
```

### Testing with Sample Clients

```bash
# Start the server
./moab_server 8080

# In another terminal, connect with netcat
nc localhost 8080

# Send registration message
REG player1 5 5 -

# Send bomb message
BOMB 5 5
```

## 📡 Message Protocol

### Client to Server

| Command | Format | Description |
|---------|--------|-------------|
| `REG` | `REG <name> <x> <y> <direction>` | Register player with ship placement |
| `BOMB` | `BOMB <x> <y>` | Attack coordinates |

### Server to Client

| Response | Format | Description |
|----------|--------|-------------|
| `WELCOME` | `WELCOME` | Registration successful |
| `TAKEN` | `TAKEN` | Username already taken |
| `INVALID` | `INVALID` | Invalid command or placement |
| `JOIN` | `JOIN <name>` | Player joined (broadcast) |
| `HIT` | `HIT <attacker> <x> <y> <victim>` | Ship hit (broadcast) |
| `MISS` | `MISS <attacker> <x> <y>` | Attack missed (broadcast) |
| `GG` | `GG <name>` | Player eliminated (broadcast) |

## 🧪 Testing

### Automated Testing

```bash
# Run the test suite
./test_server.sh 8080
```

### Manual Testing

1. Start the server: `./moab_server 8080`
2. Connect multiple clients: `nc localhost 8080`
3. Test registration, bombing, and ship destruction
4. Verify message broadcasting works correctly

## 🔧 Technical Implementation

### Key Data Structures

```c
typedef struct {
    char owner[MAX_NAME_LENGTH];
    int x, y;  // center coordinates
    char direction;  // '-' for horizontal, '|' for vertical
    int damaged_cells[SHIP_LENGTH];
    int total_damage;
} Ship;

typedef struct {
    int socket_fd;
    char name[MAX_NAME_LENGTH];
    Ship ship;
    int is_registered;
    int is_active;
} Player;
```

### Thread Safety

- Mutex-protected shared game state
- Atomic operations for player management
- Proper resource cleanup on thread exit

### Error Handling

- Socket error handling
- Memory allocation failure handling
- Client disconnection handling
- Message validation and sanitization

## 📊 Performance Features

- **Non-blocking I/O**: Uses `recv()`/`send()` for efficient network communication
- **Memory Management**: Proper allocation/deallocation of thread resources
- **Concurrent Processing**: Multiple clients can play simultaneously
- **Resource Cleanup**: Automatic cleanup of disconnected clients

## 🎓 Learning Outcomes

This project demonstrates:

- **C Programming**: Advanced C concepts including pointers, structures, and memory management
- **Network Programming**: TCP socket programming and client-server architecture
- **Concurrency**: Multi-threading with pthread library
- **System Programming**: Signal handling, file descriptors, and process management
- **Protocol Design**: Custom message protocol for game communication
- **Error Handling**: Robust error handling and resource management

## 📁 Project Structure

```
assignment4/
├── server.c              # Main server program
├── game_logic.c          # Game state management
├── client_handler.c      # Client message processing
├── server.h              # Header file with declarations
├── Makefile              # Build configuration
├── README.md             # This file
├── test_server.sh        # Automated testing script
└── manual_test_guide.md  # Manual testing instructions
```

## 🚀 Future Enhancements

- [ ] Database integration for persistent game state
- [ ] Web-based client interface
- [ ] Game statistics and leaderboards
- [ ] Spectator mode for ongoing games
- [ ] Enhanced security and input validation

## 📝 License

This project was created as part of academic coursework at the University of Toronto Scarborough.

---

**Technologies Used**: C, pthread, TCP Sockets, POSIX, Make
**Course**: CSCB09 - Software Tools and Systems Programming
**Institution**: University of Toronto Scarborough
