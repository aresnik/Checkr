/*
 * gameController.h
 *
 *      Author: alex@alexanderresnik.com
 */

#ifndef GAMECONTROLLER_H
#define GAMECONTROLLER_H

#include <SDL3/SDL.h>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include "board.h"

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
    bool isUndo = false;
};

// Represents a single move in the game history
struct MoveRecord
{
    int fromRow, fromCol;
    int toRow, toCol;
};

// Stores info about a captured piece that should stay visible until "jumped"
struct PendingCapture
{
    int row, col;
    char piece;
    int captureStep; // The path index that triggers visual removal
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

    // Handle to the background AI thread
    std::thread aiThread;

    // Constructor
    GameController();

    // Starts a visual movement animation for a checker
    void startMoveAnimation(char piece, int fromRow, int fromCol, int toRow, int toCol, bool isUndo = false);

    // Sets up a full multi-segment path animation from a MoveRecord
    void setupPathAnimation(board &b, char piece, int fR, int fC, int tR, int tC, bool isUndo = false);

    // Returns all legal moves for the selected piece
    std::vector<Square> getLegalMovesForSelection(board &b, int row, int col);

    // Handles mouse clicks and move selection logic
    void handleClick(board &b, int row, int col, std::vector<MoveRecord> &history, int &historyIndex);

    // Checks if a selected piece belongs to the current player
    bool isCurrentPlayersPiece(const board &b, int row, int col) const;

    // Updates AI state and applies completed AI moves
    void updateAI(board &b, std::vector<MoveRecord> &history, int &historyIndex);

    // Path of squares used for multi-jump animations
    std::vector<Square> animationPath;

    // List of pieces currently being "jumped" in the active animation
    std::vector<PendingCapture> pendingCaptures;

    // Current position within the animation path
    int animationPathIndex = 0;

    // Updates animation progress each frame
    void updateAnimation();

    // Ensures the AI thread is safely shut down
    void stopAI();
};

#endif
