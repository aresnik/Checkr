/*
 * gameController.cpp
 *
 *      Author: Alexander Resnik
 */

#include "gameController.h"
#include "board.h"
#include <iostream>
#include <thread>

GameController::GameController()
    : aiThinking(false), aiMoveReady(false)
{
    aiFrom = {-1, -1};
    aiTo = {-1, -1};
}

void GameController::startMoveAnimation(char piece, int fromRow, int fromCol, int toRow, int toCol)
{
    animation.active = true;
    animation.piece = piece;
    animation.fromRow = fromRow;
    animation.fromCol = fromCol;
    animation.toRow = toRow;
    animation.toCol = toCol;
    animation.startTime = SDL_GetTicks();
    animation.durationMs = 300;
}

std::vector<Square> GameController::getLegalMovesForSelection(board &b, int row, int col)
{
    std::vector<Square> legalMoves;
    std::vector<std::pair<int, int>> destinations =
        b.getLegalDestinationsForSquare(row, col);

    for (size_t i = 0; i < destinations.size(); ++i)
    {
        Square sq;
        sq.row = destinations[i].first;
        sq.col = destinations[i].second;
        legalMoves.push_back(sq);
    }

    return legalMoves;
}

void GameController::handleClick(board &b, int row, int col)
{
    if (animation.active || aiThinking || b.getTurnPublic() == 'b')
        return;

    if (row < 0 || row >= 8 || col < 0 || col >= 8)
        return;

    if ((row + col) % 2 == 0)
        return;

    if (selectedRow == -1 && selectedCol == -1)
    {
        if (isCurrentPlayersPiece(b, row, col))
        {
            selectedRow = row;
            selectedCol = col;
            legalMoves = getLegalMovesForSelection(b, row, col);

            std::cout << "Selected piece: (" << row << ", " << col << ")\n";
        }
    }
    else
    {
        if (isCurrentPlayersPiece(b, row, col))
        {
            selectedRow = row;
            selectedCol = col;
            legalMoves = getLegalMovesForSelection(b, row, col);

            std::cout << "Reselected piece: (" << row << ", " << col << ")\n";
        }
        else
        {
            char movingPiece = b.getPieceAt8x8(selectedRow, selectedCol);

            // NEW: fetch full move path before applying the move
            std::vector<std::pair<int, int>> pathPairs =
                b.getMovePath8x8(selectedRow, selectedCol, row, col);

            bool moved = b.tryMove8x8(selectedRow, selectedCol, row, col);

            if (moved)
            {
                std::cout << "Moved from (" << selectedRow << ", " << selectedCol
                          << ") to (" << row << ", " << col << ")\n";

                // NEW: store path in controller
                animationPath.clear();
                animationPathIndex = 1;

                for (size_t i = 0; i < pathPairs.size(); ++i)
                {
                    Square sq;
                    sq.row = pathPairs[i].first;
                    sq.col = pathPairs[i].second;
                    animationPath.push_back(sq);
                }

                // For now, animate only the first segment if available.
                if (animationPath.size() >= 2)
                {
                    startMoveAnimation(movingPiece,
                                       animationPath[0].row, animationPath[0].col,
                                       animationPath[1].row, animationPath[1].col);
                }
                else
                {
                    // Fallback
                    startMoveAnimation(movingPiece,
                                       selectedRow, selectedCol,
                                       row, col);
                }
            }
            else
            {
                std::cout << "Illegal move from (" << selectedRow << ", "
                          << selectedCol << ") to (" << row << ", " << col << ")\n";
            }

            selectedRow = -1;
            selectedCol = -1;
            legalMoves.clear();

            if (moved && b.getTurnPublic() == 'b' && !aiThinking)
            {
                aiThinking = true;

                board boardCopy = b;

                std::thread([boardCopy, this]() mutable
                            {
                    int fr, fc, tr, tc;
                    bool found = boardCopy.findBestMove8x8(3, fr, fc, tr, tc);

                    if (found)
                    {
                        std::lock_guard<std::mutex> lock(aiMutex);
                        aiFrom = {fr, fc};
                        aiTo = {tr, tc};
                        aiMoveReady = true;
                    }

                    aiThinking = false; })
                    .detach();
            }
        }
    }
}

bool GameController::isCurrentPlayersPiece(const board &b, int row, int col) const
{
    char piece = b.getPieceAt8x8(row, col);
    char turn = b.getTurnPublic();

    if (turn == 'b')
        return (piece == 'b' || piece == 'B');
    else
        return (piece == 'r' || piece == 'R');
}

void GameController::updateAI(board &b)
{
    if (aiMoveReady && !animation.active)
    {
        Square from;
        Square to;

        {
            std::lock_guard<std::mutex> lock(aiMutex);
            from = aiFrom;
            to = aiTo;
            aiMoveReady = false;
        }

        char movingPiece = b.getPieceAt8x8(from.row, from.col);

        // NEW: fetch full path before applying move
        std::vector<std::pair<int, int>> pathPairs =
            b.getMovePath8x8(from.row, from.col, to.row, to.col);

        bool aiApplied = b.tryMove8x8(from.row, from.col, to.row, to.col);

        if (aiApplied)
        {
            std::cout << "AI moved from (" << from.row << ", " << from.col
                      << ") to (" << to.row << ", " << to.col << ")\n";

            // NEW: store path
            animationPath.clear();
            animationPathIndex = 1;

            for (size_t i = 0; i < pathPairs.size(); ++i)
            {
                Square sq;
                sq.row = pathPairs[i].first;
                sq.col = pathPairs[i].second;
                animationPath.push_back(sq);
            }

            // For now, animate only the first segment if available.
            if (animationPath.size() >= 2)
            {
                startMoveAnimation(movingPiece,
                                   animationPath[0].row, animationPath[0].col,
                                   animationPath[1].row, animationPath[1].col);
            }
            else
            {
                startMoveAnimation(movingPiece,
                                   from.row, from.col,
                                   to.row, to.col);
            }
        }
        else
        {
            std::cout << "AI move failed to apply.\n";
        }
    }
}

void GameController::updateAnimation()
{
    // If a segment is still animating, do nothing.
    if (animation.active)
        return;

    // No path or only one point means nothing to continue.
    if (animationPath.size() < 2)
        return;

    // If we already reached the last segment, clear the path state.
    if (animationPathIndex >= static_cast<int>(animationPath.size()) - 1)
    {
        animationPath.clear();
        animationPathIndex = 0;
        return;
    }

    // Start the next segment in the stored path.
    char piece = animation.piece;

    int fromIndex = animationPathIndex;
    int toIndex = animationPathIndex + 1;

    startMoveAnimation(piece,
                       animationPath[fromIndex].row,
                       animationPath[fromIndex].col,
                       animationPath[toIndex].row,
                       animationPath[toIndex].col);

    animationPathIndex++;
}