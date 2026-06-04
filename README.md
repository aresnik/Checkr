# Checkers GUI frontend

A modern GUI-based Checkers game frontend written in C++ using SDL3.

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
- Modern event-driven architecture
- Cross-platform C++ project structure

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

## main Class (SDL Layer / View)

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

---

# Project Status

This project is currently under active development as part of an ongoing modernization and refactoring effort from a legacy terminal-based engine into a fully featured graphical Checkers application.
