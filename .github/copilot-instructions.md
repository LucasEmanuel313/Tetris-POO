# Tetris-POO AI Coding Instructions

## Project Overview
This is an object-oriented Tetris game implemented in C++ with multiplayer networking support. The game features single-player and multiplayer modes, where players compete by sending "garbage" lines to opponents when clearing multiple lines.

## Architecture
- **Core Classes**:
  - `table`: Manages the 10x22 game grid, current block positioning, collision detection, line clearing, and scoring
  - `block`: Represents Tetris pieces with 4x4 char arrays and rotation logic
- **Networking**: Uses Winsock for TCP connections. Server relays game events between two clients.
- **Game Modes**: Single-player (local), host multiplayer (embedded server), join multiplayer (connect to remote server)

## Key Components & Data Flow
- **Game Loop**: In `main.cpp`, handles input (a/d/s/w/space), automatic falling (500ms intervals), and network message processing
- **Multiplayer Protocol**:
  - Clients send "CLEARED X" when clearing X lines
  - Server calculates garbage (1 line cleared → 0 garbage, 2→1, 3→2, 4→4) and sends "GARBAGE Y" to opponent
  - Game ends with "GAMEOVER" → "YOU_WIN" to opponent
- **Collision System**: Uses `block_clear()` and `update_table()` to temporarily remove/replace blocks for movement checks

## Build & Run
- **Build Client**: `make` (uses Makefile.txt: g++ -std=c++17 -lws2_32 main.cpp table.cpp -o game.exe)
- **Build Server**: `g++ -std=c++17 -lws2_32 server.cpp -o server.exe`
- **Run**: Execute `game.exe` for menu-driven gameplay. Server runs standalone on port 5555.

## Coding Patterns
- **Piece Definitions**: Tetris pieces stored as `const char [4][4]` arrays in `block.h` (e.g., `PIECE_I`, `PIECE_O`)
- **Grid Representation**: `char positions[10][22]` in `table` class, '#' for filled, ' ' for empty
- **Rotation**: `block::rotate()` modifies the 4x4 array in-place
- **Profile Calculation**: `block::make_profile()` computes lowest row per column for efficient collision checks
- **Memory Management**: Manual `new`/`delete` for `current_block`; destructor cleans up
- **Networking Helpers**: `sendLine()` appends '\n', non-blocking recv with buffer parsing

## Conventions
- **Platform**: Windows-only (Winsock, conio.h for input, system("CLS") for clear)
- **Input Handling**: `_kbhit()` + `_getch()` for real-time controls (a=left, d=right, s=down, w=rotate, space=drop, q=quit)
- **Scoring**: 1 line=100pts, 2=300, 3=500, 4=800
- **Garbage Logic**: Applied by shifting grid up and adding a row with one random hole
- **Error Handling**: Simple `std::cerr` output; network errors disconnect clients

## Common Tasks
- **Adding New Pieces**: Define new `const char [4][4]` in `block.h`, add to `TETROMINOES` array
- **Modifying Mechanics**: Update `handle_landing()` in `table.h` for scoring/rules, `garbageFromCleared()` for multiplayer balance
- **Network Features**: Extend message types in `processIncoming()` and server relay logic
- **UI Changes**: Modify `print_table()` for custom display (currently console-based with | separators)

## Dependencies
- **Libraries**: ws2_32 (Winsock), windows.h, conio.h
- **Compiler**: MinGW g++ with C++17 support
- **No External Deps**: Pure C++ with standard library

Reference `table.h` for game logic, `main.cpp` for integration, `server.cpp` for networking examples.</content>
<parameter name="filePath">c:\Users\xfalc\Documents\GitHub\Tetris-POO\.github\copilot-instructions.md