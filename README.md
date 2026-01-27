# Tetris-POO (C++/SFML)

Tetris project written in **C++17** using **SFML 3.0.2**, with **Singleplayer** and **Multiplayer** (simple TCP client + server).

## Project structure

- `src/`
  - `main.cpp`: main loop (state machine: MENU / SINGLEPLAYER / MULTIPLAYER / GAME_OVER).
  - `Game.cpp`: singleplayer rendering + input (SFML) on top of the `table` model.
  - `Multiplayer.cpp`: multiplayer screen + networking (non-blocking connect) + local match logic.
  - `table.cpp`: **game model** (board, active piece, collisions, line clears, hold/next, board serialization).
  - `server.cpp`: simple 2-player TCP server (Win32 thread + `select()`; line-based protocol).
  - `net_client.cpp`: client-side helpers (send/receive line messages).
  - `WindowManager.cpp`: SFML window creation/config.
  - `SinglePlayerState.cpp`: auxiliary singleplayer state (when applicable).

- `include/`
  - `block.h`: `block` definition (base class) + derived tetromino blocks via `TetrominoBlock<Type>`.
  - `table.h`: `table` model interface (owns the active piece via `std::unique_ptr<block>`).
  - `Multiplayer.h`, `Game.h`, `MainMenu.h`, etc.: headers for screens/controllers/UI.

- `assets/`
  - `Tetris.ttf`: font.
  - `Images/Background_Tetris.jpg`: background.

- `bin/`
  - Executable outputs (`game.exe`, `debug.exe`).

- `build/`
  - Build intermediates/objects.

## How the code is organized 

- **Model**: `table` (board + rules) in `include/table.h` and `src/table.cpp`.
- **View/Controller**:
  - `SfmlGame` (singleplayer) in `include/Game.h` and `src/Game.cpp`.
  - `SfmlMultiplayer` (multiplayer) in `include/Multiplayer.h` and `src/Multiplayer.cpp`.
- **Networking**:
  - Client helpers: `src/net_client.cpp`.
  - Server: `src/server.cpp`.

### OOP example (inheritance and polymorphism)

- Inheritance: `TetrominoBlock<Type> : public block` in `include/block.h`.
- Polymorphism: `block` has virtual methods (e.g., `rotate()`), and the game stores the current piece through a base-class pointer (`std::unique_ptr<block>` inside `table`).

## Building 

### Prerequisites

- Windows
- MinGW-w64 with `g++` (C++17)
- SFML 3.0.2 installed at `C:/SFML-3.0.2` (or adjust the path)

> Important: the project uses Winsock, so you must link with `-lws2_32`.

### Option A) Using `MakeFile.txt`

This repository includes a simple makefile in `MakeFile.txt`.

1) Make sure SFML is set to the correct path:

- In `MakeFile.txt`:
  - `SFML_DIR := C:/SFML-3.0.2`

2) In a terminal at the project root, run:

- Windowed release build:
  - `make -f MakeFile.txt game`

- Debug build (with console):
  - `make -f MakeFile.txt debug`

- Both:
  - `make -f MakeFile.txt all`

Outputs:
- `bin/game.exe`
- `bin/debug.exe`

### Option B) Manual build (PowerShell)

Adjust `C:\SFML-3.0.2` if needed and run from the project root:

```powershell
& "C:\MinGW\bin\g++.exe" -g -std=c++17 -Iinclude -IC:\SFML-3.0.2\include \
  src\main.cpp src\WindowManager.cpp src\Game.cpp src\Multiplayer.cpp \
  src\table.cpp src\net_client.cpp src\server.cpp src\SinglePlayerState.cpp \
  -LC:\SFML-3.0.2\lib -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio -lws2_32 \
  -mwindows -o bin\game.exe
```

To produce the console version (useful for logs), replace `-mwindows` with `-mconsole` and output to `bin\debug.exe`.

## Running

- Run `bin/game.exe`.
- If assets fail to load (font/image), run the executable from the project root so relative paths like `assets/...` resolve correctly.

---

If you want, I can also add a short "Multiplayer protocol" section (messages like `BOARD`, `GARBAGE`, `START`, `LEAVE`) to document the networking mode.
