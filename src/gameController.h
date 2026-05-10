/*
 * gameController.h
 *
 *      Author: Alexander Resnik
 */

#ifndef GAMECONTROLLER_H
#define GAMECONTROLLER_H

#include <SDL3/SDL.h>
#include <vector>
#include <atomic>
#include <mutex>
#include "board.h"

struct Square
{
    int row;
    int col;
};

// Stores all data related to animating a checker movement
struct MoveAnimation
{
    bool active = false;

    // The piece being animated ('r', 'b', 'R', 'B')
    char piece = 'e';

    int fromRow = -1;
    int fromCol = -1;

    int toRow = -1;
    int toCol = -1;

    Uint32 startTime = 0;
    Uint32 durationMs = 2000;
};

// Handles user input, animations, and AI coordination
class GameController
{
public:
    // Currently selected checker position
    int selectedRow = -1;
    int selectedCol = -1;

    // List of valid move destinations for the selected piece
    std::vector<Square> legalMoves;

    // Current move animation state
    MoveAnimation animation;

    // Thread-safe flags used for AI processing
    std::atomic<bool> aiThinking;
    std::atomic<bool> aiMoveReady;

    // Mutex protects shared AI move data between threads
    std::mutex aiMutex;

    // AI move start and destination positions
    Square aiFrom;
    Square aiTo;

    // Constructor
    GameController();

    // Starts a visual movement animation for a checker
    void startMoveAnimation(char piece, int fromRow, int fromCol, int toRow, int toCol);

    // Returns all legal moves for the selected piece
    std::vector<Square> getLegalMovesForSelection(board &b, int row, int col);

    // Handles mouse clicks and move selection logic
    void handleClick(board &b, int row, int col);

    // Checks if a selected piece belongs to the current player
    bool isCurrentPlayersPiece(const board &b, int row, int col) const;

    // Updates AI state and applies completed AI moves
    void updateAI(board &b);

    // Path of squares used for multi-jump animations
    std::vector<Square> animationPath;

    // Current position within the animation path
    int animationPathIndex = 0;

    // Updates animation progress each frame
    void updateAnimation();
};

#endif
