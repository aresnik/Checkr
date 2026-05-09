//============================================================================
// Name        : main.cpp
// Author      : Alexander Resnik
//============================================================================

#include <SDL3/SDL.h>
#include <iostream>
#include <vector>
#include "board.h"
#include "gameController.h"

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 800;
const int BOARD_SIZE = 8;
const int TILE_SIZE = WINDOW_WIDTH / BOARD_SIZE;

void drawCheckerboard(SDL_Renderer *renderer)
{
    for (int row = 0; row < BOARD_SIZE; ++row)
    {
        for (int col = 0; col < BOARD_SIZE; ++col)
        {
            // SDL3 uses SDL_FRect for rendering
            SDL_FRect tile;
            tile.x = (float)(col * TILE_SIZE);
            tile.y = (float)(row * TILE_SIZE);
            tile.w = (float)TILE_SIZE;
            tile.h = (float)TILE_SIZE;

            bool isDarkSquare = ((row + col) % 2 == 1);

            if (isDarkSquare)
                SDL_SetRenderDrawColor(renderer, 139, 69, 19, 255);
            else
                SDL_SetRenderDrawColor(renderer, 245, 222, 179, 255);

            SDL_RenderFillRect(renderer, &tile);
        }
    }
}

void drawFilledCircle(SDL_Renderer *renderer, int centerX, int centerY, int radius)
{
    for (int dy = -radius; dy <= radius; ++dy)
    {
        for (int dx = -radius; dx <= radius; ++dx)
        {
            if (dx * dx + dy * dy <= radius * radius)
                // Renamed in SDL3
                SDL_RenderPoint(renderer, (float)(centerX + dx), (float)(centerY + dy));
        }
    }
}

void drawPieceAtPixel(SDL_Renderer *renderer, int centerX, int centerY, char piece)
{
    int radius = TILE_SIZE / 2 - 12;

    if (piece == 'b' || piece == 'B')
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
    else if (piece == 'r' || piece == 'R')
        SDL_SetRenderDrawColor(renderer, 200, 40, 40, 255);
    else
        return;

    drawFilledCircle(renderer, centerX, centerY, radius);

    if (piece == 'B' || piece == 'R')
    {
        SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
        SDL_FRect kingMark;
        kingMark.x = (float)(centerX - 14);
        kingMark.y = (float)(centerY - 14);
        kingMark.w = 28.0f;
        kingMark.h = 28.0f;
        // Renamed in SDL3
        SDL_RenderRect(renderer, &kingMark);
    }
}

void drawPiece(SDL_Renderer *renderer, int row, int col, char piece)
{
    int centerX = col * TILE_SIZE + TILE_SIZE / 2;
    int centerY = row * TILE_SIZE + TILE_SIZE / 2;
    drawPieceAtPixel(renderer, centerX, centerY, piece);
}

void drawPieces(SDL_Renderer *renderer, const board &b, const MoveAnimation &animation)
{
    for (int row = 0; row < 8; ++row)
    {
        for (int col = 0; col < 8; ++col)
        {
            char piece = b.getPieceAt8x8(row, col);

            if (piece == 'e' || piece == 'x')
                continue;

            if (animation.active &&
                row == animation.toRow &&
                col == animation.toCol)
            {
                continue;
            }

            drawPiece(renderer, row, col, piece);
        }
    }
}

void drawSelectedSquare(SDL_Renderer *renderer, int selectedRow, int selectedCol)
{
    if (selectedRow < 0 || selectedCol < 0)
        return;

    SDL_FRect highlight;
    highlight.x = (float)(selectedCol * TILE_SIZE);
    highlight.y = (float)(selectedRow * TILE_SIZE);
    highlight.w = (float)TILE_SIZE;
    highlight.h = (float)TILE_SIZE;

    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderRect(renderer, &highlight);

    SDL_FRect inner;
    inner.x = highlight.x + 2.0f;
    inner.y = highlight.y + 2.0f;
    inner.w = highlight.w - 4.0f;
    inner.h = highlight.h - 4.0f;
    SDL_RenderRect(renderer, &inner);
}

void drawLegalMoves(SDL_Renderer *renderer, const std::vector<Square> &legalMoves)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);

    for (size_t i = 0; i < legalMoves.size(); ++i)
    {
        int centerX = legalMoves[i].col * TILE_SIZE + TILE_SIZE / 2;
        int centerY = legalMoves[i].row * TILE_SIZE + TILE_SIZE / 2;
        int radius = 12;

        drawFilledCircle(renderer, centerX, centerY, radius);
    }
}

void drawMoveAnimation(SDL_Renderer *renderer, MoveAnimation &animation)
{
    if (!animation.active)
        return;

    Uint64 now = SDL_GetTicks(); // Now returns Uint64
    Uint64 elapsed = now - animation.startTime;

    if (elapsed >= (Uint64)animation.durationMs)
    {
        animation.active = false;
        return;
    }

    float t = static_cast<float>(elapsed) / static_cast<float>(animation.durationMs);

    float fromX = (float)(animation.fromCol * TILE_SIZE + TILE_SIZE / 2.0f);
    float fromY = (float)(animation.fromRow * TILE_SIZE + TILE_SIZE / 2.0f);
    float toX = (float)(animation.toCol * TILE_SIZE + TILE_SIZE / 2.0f);
    float toY = (float)(animation.toRow * TILE_SIZE + TILE_SIZE / 2.0f);

    float currentX = fromX + (toX - fromX) * t;
    float currentY = fromY + (toY - fromY) * t;

    drawPieceAtPixel(renderer,
                     static_cast<int>(currentX),
                     static_cast<int>(currentY),
                     animation.piece);
}

void drawThinkingIndicator(SDL_Renderer *renderer, bool aiThinking)
{
    if (!aiThinking)
        return;

    SDL_FRect box;
    box.x = 20;
    box.y = 20;
    box.w = 180;
    box.h = 40;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_RenderFillRect(renderer, &box);

    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderRect(renderer, &box);

    // Simple animated dots / spinner without text rendering
    Uint32 ticks = SDL_GetTicks();
    int dotCount = (ticks / 300) % 7;

    for (int i = 0; i < dotCount; i++)
    {
        SDL_FRect dot;
        dot.x = 45 + i * 25;
        dot.y = 32;
        dot.w = 10;
        dot.h = 10;

        SDL_RenderFillRect(renderer, &dot);
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

int main(int argc, char *argv[])
{
    if (!SDL_Init(SDL_INIT_VIDEO)) // SDL3 returns bool
    {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << '\n';
        return 1;
    }

    // SDL3 window creation simplified
    SDL_Window *window = SDL_CreateWindow("Checkers", WINDOW_WIDTH, WINDOW_HEIGHT, 0);

    if (!window)
    {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << '\n';
        SDL_Quit();
        return 1;
    }

    // SDL3 renderer creation simplified (no -1 index or accelerated flag)
    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer)
    {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << '\n';
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    board b;
    GameController controller;

    bool running = true;
    SDL_Event event;

    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            // Renamed events in SDL3
            if (event.type == SDL_EVENT_QUIT)
            {
                running = false;
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                if (event.button.button == SDL_BUTTON_LEFT)
                {
                    float mouseX = event.button.x;
                    float mouseY = event.button.y;

                    int col = (int)(mouseX / TILE_SIZE);
                    int row = (int)(mouseY / TILE_SIZE);

                    controller.handleClick(b, row, col);
                }
            }
        }

        controller.updateAI(b);
        controller.updateAnimation();

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        drawCheckerboard(renderer);
        drawSelectedSquare(renderer, controller.selectedRow, controller.selectedCol);
        drawLegalMoves(renderer, controller.legalMoves);
        drawPieces(renderer, b, controller.animation);
        drawMoveAnimation(renderer, controller.animation);
        drawThinkingIndicator(renderer, controller.aiThinking);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
