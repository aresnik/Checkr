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

struct MoveAnimation
{
    bool active = false;
    char piece = 'e';

    int fromRow = -1;
    int fromCol = -1;
    int toRow = -1;
    int toCol = -1;

    Uint32 startTime = 0;
    Uint32 durationMs = 2000;
};

class GameController
{
public:
    int selectedRow = -1;
    int selectedCol = -1;
    std::vector<Square> legalMoves;

    MoveAnimation animation;

    std::atomic<bool> aiThinking;
    std::atomic<bool> aiMoveReady;

    std::mutex aiMutex;
    Square aiFrom;
    Square aiTo;

    GameController();

    void startMoveAnimation(char piece, int fromRow, int fromCol, int toRow, int toCol);

    std::vector<Square> getLegalMovesForSelection(board &b, int row, int col);

    void handleClick(board &b, int row, int col);
    bool isCurrentPlayersPiece(const board &b, int row, int col) const;
    void updateAI(board &b);

    std::vector<Square> animationPath;
    int animationPathIndex = 0;
    void updateAnimation();
};

#endif