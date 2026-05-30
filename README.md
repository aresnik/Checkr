# Checkers in C++

A modern GUI-based Checkers game written in C++ using SDL3.

This project began as a classic terminal-based Checkers AI and has since been transformed into a modern event-driven application featuring a graphical user interface, asynchronous AI processing, animations, and a modular architecture.

## Features

- Graphical user interface using SDL3
- Human vs Computer gameplay
- Human vs Human gameplay (comming)
- Computer vs Computer gameplay(comming)
- Mouse-based piece selection and movement
- Animated checker movement
- Forced jump enforcement
- King promotion
- Asynchronous AI processing (GUI remains responsive while AI searches)
- Iterative deepening minimax AI
- Alpha-beta pruning
- Heuristic board evaluation
- Modern event-driven architecture
- Cross-platform C++ project structure

---

# How It Works

The game engine uses a compact 8 x 4 board representation internally instead of storing all 64 squares. Since only the dark squares are playable in Checkers, only 32 positions are required.

The AI uses:

- Minimax search
- Alpha-beta pruning
- Iterative deepening
- Heuristic evaluation functions

The evaluation system considers:

- Number of remaining pieces
- Kings vs normal pieces
- Piece positioning
- Advancement toward promotion
- Endgame positioning
- Mobility and available moves

Terminal game states occur when:

1. Red has no remaining pieces
2. Black has no remaining pieces
3. Red has no legal moves
4. Black has no legal moves

---

# Architecture

## board Class (Game Engine / Model)

The `board` class represents the complete logical game state.

Responsibilities include:

- Board representation
- Move generation
- Jump generation
- Forced jump logic
- Move validation
- Applying and undoing moves
- Turn management
- Win/loss detection
- Board evaluation support

The board internally stores legal moves and supports efficient move simulation for AI search.

---

## move Class

The `move` class represents a complete move sequence, including:

- Standard moves
- Single jumps
- Multiple jumps

It stores all information required to apply or undo a move during gameplay or AI search.

---

## GameController Class

The `GameController` class manages overall game flow and orchestration between the engine and the graphical interface.

Responsibilities include:

- Handling player turns
- Coordinating AI turns
- Launching asynchronous AI search threads
- Managing move animations
- Tracking game state
- Synchronizing gameplay with rendering

The AI operates on copies of the board state to prevent blocking or corrupting the active game state during search.

---

## window Class (SDL Layer / View)

The `window` class handles all SDL3-related functionality.

Responsibilities include:

- Window creation
- Rendering the checkerboard
- Drawing pieces
- Mouse input handling
- Highlighting legal moves
- Animation rendering
- Texture management
- Event processing

The rendering system is fully event-driven and continuously updates while the AI searches in the background.

---

# Artificial Intelligence

The AI uses iterative deepening with alpha-beta pruning to search possible game states efficiently.

The search system:

- Expands legal moves recursively
- Evaluates resulting board states
- Uses pruning to eliminate unnecessary branches
- Selects the best move according to the heuristic evaluation function

The asynchronous AI system allows the computer player to think on a separate thread while the GUI remains responsive.

---

# Technologies Used

- C++
- SDL3
- Standard Library threading (`std::async`, `std::future`)
- Modern event-driven architecture
- Object-oriented design

---

# Future Improvements

Planned and experimental features include:

- Improved AI evaluation heuristics
- Enhanced move animations
- Sound effects and music
- AI difficulty settings
- Opening book support
- Transposition tables
- Network multiplayer
- Mobile platform support

---

# Building the Project

The project is built using:

- C++
- SDL3
- Makefiles
- Visual Studio Code

Typical build command:

```bash
make
```

Run the executable:

```bash
./bin/checkers
```

---

# Project Status

This project is currently under active development as part of an ongoing modernization and refactoring effort from a legacy terminal-based engine into a fully featured graphical Checkers application.