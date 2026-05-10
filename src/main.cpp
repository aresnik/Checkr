//============================================================================
// Name        : main.cpp
// Author      : Alexander Resnik
//============================================================================

#include <SDL3/SDL.h>
#include <iostream>
#include <vector>
#include "board.h"
#include "gameController.h"

// Window and board layout constants.
// The board is 8x8, and since the window is 800x800, each square is 100x100 pixels.
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 800;
const int BOARD_SIZE = 8;
const int TILE_SIZE = WINDOW_WIDTH / BOARD_SIZE;

// Draws the 8x8 checkerboard background.
// This function only draws the squares, not the pieces.
void drawCheckerboard(SDL_Renderer *renderer)
{
    // Loop through every row and column on the visual 8x8 board.
    for (int row = 0; row < BOARD_SIZE; ++row)
    {
        for (int col = 0; col < BOARD_SIZE; ++col)
        {
            // SDL3 uses SDL_FRect for rendering rectangles with floating-point positions.
            SDL_FRect tile;
            tile.x = (float)(col * TILE_SIZE);
            tile.y = (float)(row * TILE_SIZE);
            tile.w = (float)TILE_SIZE;
            tile.h = (float)TILE_SIZE;

            // Checkerboards alternate colors.
            // If row + col is odd, this is one of the playable dark squares.
            bool isDarkSquare = ((row + col) % 2 == 1);

            // Brown for dark squares, light tan for light squares.
            if (isDarkSquare)
                SDL_SetRenderDrawColor(renderer, 139, 69, 19, 255);
            else
                SDL_SetRenderDrawColor(renderer, 245, 222, 179, 255);

            // Fill the current square with whichever color was just selected.
            SDL_RenderFillRect(renderer, &tile);
        }
    }
}

// Draws a filled circle by plotting every point inside the circle radius.
// SDL does not provide a built-in filled-circle primitive, so this manually draws one pixel at a time.
void drawFilledCircle(SDL_Renderer *renderer, int centerX, int centerY, int radius)
{
    // Walk through a square bounding box around the circle.
    for (int dy = -radius; dy <= radius; ++dy)
    {
        for (int dx = -radius; dx <= radius; ++dx)
        {
            // The circle equation is x^2 + y^2 <= r^2.
            // Points satisfying this condition are inside the circle.
            if (dx * dx + dy * dy <= radius * radius)
                // SDL_RenderPoint is the SDL3 name for drawing a single point.
                SDL_RenderPoint(renderer, (float)(centerX + dx), (float)(centerY + dy));
        }
    }
}

// Draws one checker piece at an exact pixel position.
// This is used both for normal pieces sitting on board squares and for animated pieces between squares.
void drawPieceAtPixel(SDL_Renderer *renderer, int centerX, int centerY, char piece)
{
    // Leave some padding around the checker so it does not fill the entire tile.
    int radius = TILE_SIZE / 2 - 12;

    // Lowercase pieces are regular pieces.
    // Uppercase pieces are kings.
    // Black pieces are drawn dark; red pieces are drawn red.
    if (piece == 'b' || piece == 'B')
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
    else if (piece == 'r' || piece == 'R')
        SDL_SetRenderDrawColor(renderer, 200, 40, 40, 255);
    else
        return; // Do not draw anything for empty or invalid piece characters.

    // Draw the main checker body.
    drawFilledCircle(renderer, centerX, centerY, radius);

    // Kings get a small gold square outline in the middle of the piece.
    if (piece == 'B' || piece == 'R')
    {
        SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
        SDL_FRect kingMark;
        kingMark.x = (float)(centerX - 14);
        kingMark.y = (float)(centerY - 14);
        kingMark.w = 28.0f;
        kingMark.h = 28.0f;
        // SDL_RenderRect is the SDL3 name for drawing a rectangle outline.
        SDL_RenderRect(renderer, &kingMark);
    }
}

// Converts a board row/column into the pixel center of that square,
// then draws the piece there.
void drawPiece(SDL_Renderer *renderer, int row, int col, char piece)
{
    int centerX = col * TILE_SIZE + TILE_SIZE / 2;
    int centerY = row * TILE_SIZE + TILE_SIZE / 2;
    drawPieceAtPixel(renderer, centerX, centerY, piece);
}

// Draws all pieces from the current board state.
// The animation parameter is used to avoid drawing a duplicate piece while it is moving.
void drawPieces(SDL_Renderer *renderer, const board &b, const MoveAnimation &animation)
{
    for (int row = 0; row < 8; ++row)
    {
        for (int col = 0; col < 8; ++col)
        {
            // Ask the board model what piece is at this visual 8x8 coordinate.
            char piece = b.getPieceAt8x8(row, col);

            // 'e' means empty and 'x' usually means an invalid / unused square.
            // Neither one should be drawn as a checker.
            if (piece == 'e' || piece == 'x')
                continue;

            // If a move animation is active, the destination square already contains
            // the moved piece in the board state. Skip drawing that stationary version
            // so the animated moving piece is the only visible copy.
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
    // A negative selected row/column means nothing is selected.
    if (selectedRow < 0 || selectedCol < 0)
        return;

    // Outer highlight rectangle around the selected tile.
    SDL_FRect highlight;
    highlight.x = (float)(selectedCol * TILE_SIZE);
    highlight.y = (float)(selectedRow * TILE_SIZE);
    highlight.w = (float)TILE_SIZE;
    highlight.h = (float)TILE_SIZE;

    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderRect(renderer, &highlight);

    // Draw a second rectangle slightly inset so the highlight appears thicker.
    SDL_FRect inner;
    inner.x = highlight.x + 2.0f;
    inner.y = highlight.y + 2.0f;
    inner.w = highlight.w - 4.0f;
    inner.h = highlight.h - 4.0f;
    SDL_RenderRect(renderer, &inner);
}

// Draws blue dots on squares where the selected piece is allowed to move.
void drawLegalMoves(SDL_Renderer *renderer, const std::vector<Square> &legalMoves)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);

    // Each Square stores a row and column destination.
    for (size_t i = 0; i < legalMoves.size(); ++i)
    {
        int centerX = legalMoves[i].col * TILE_SIZE + TILE_SIZE / 2;
        int centerY = legalMoves[i].row * TILE_SIZE + TILE_SIZE / 2;
        int radius = 12;

        drawFilledCircle(renderer, centerX, centerY, radius);
    }
}

// Draws the currently moving piece between its starting square and ending square.
// The board state may already be updated, but this function creates the visual movement.
void drawMoveAnimation(SDL_Renderer *renderer, MoveAnimation &animation)
{
    // Nothing to animate if the animation flag is off.
    if (!animation.active)
        return;

    // SDL_GetTicks returns the number of milliseconds since SDL was initialized.
    Uint64 now = SDL_GetTicks();
    Uint64 elapsed = now - animation.startTime;

    // Once the animation duration has passed, stop animating.
    if (elapsed >= (Uint64)animation.durationMs)
    {
        animation.active = false;
        return;
    }

    // t moves from 0.0 to 1.0 over the course of the animation.
    // 0.0 means the piece is at the starting square.
    // 1.0 means the piece has reached the destination square.
    float t = static_cast<float>(elapsed) / static_cast<float>(animation.durationMs);

    // Convert start and end board coordinates into pixel-center coordinates.
    float fromX = (float)(animation.fromCol * TILE_SIZE + TILE_SIZE / 2.0f);
    float fromY = (float)(animation.fromRow * TILE_SIZE + TILE_SIZE / 2.0f);
    float toX = (float)(animation.toCol * TILE_SIZE + TILE_SIZE / 2.0f);
    float toY = (float)(animation.toRow * TILE_SIZE + TILE_SIZE / 2.0f);

    // Linear interpolation: current = start + (end - start) * t.
    // This creates smooth movement from the starting square to the ending square.
    float currentX = fromX + (toX - fromX) * t;
    float currentY = fromY + (toY - fromY) * t;

    // Draw the moving piece at its interpolated pixel position.
    drawPieceAtPixel(renderer,
                     static_cast<int>(currentX),
                     static_cast<int>(currentY),
                     animation.piece);
}

// Draws a simple visual indicator while the AI is calculating its move.
// This avoids needing SDL_ttf or text rendering.
void drawThinkingIndicator(SDL_Renderer *renderer, bool aiThinking)
{
    // Do nothing unless the controller says the AI is currently thinking.
    if (!aiThinking)
        return;

    // Semi-transparent status box in the top-left corner.
    SDL_FRect box;
    box.x = 20;
    box.y = 20;
    box.w = 180;
    box.h = 40;

    // Enable alpha blending so the black box can be partially transparent.
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_RenderFillRect(renderer, &box);

    // Green outline around the thinking box.
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderRect(renderer, &box);

    // Simple animated dots / spinner without text rendering.
    // The number of dots changes over time based on SDL ticks.
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

    // Restore normal non-blended drawing for later rendering.
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

int main(int argc, char *argv[])
{
    // Initialize SDL's video subsystem.
    // SDL3 returns true on success and false on failure.
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << '\n';
        return 1;
    }

    // Create the game window.
    // In SDL3, the window creation call is simpler than SDL2.
    SDL_Window *window = SDL_CreateWindow("Checkers", WINDOW_WIDTH, WINDOW_HEIGHT, 0);

    // If the window could not be created, clean up SDL and exit.
    if (!window)
    {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << '\n';
        SDL_Quit();
        return 1;
    }

    // Create the renderer used for all drawing.
    // In SDL3, this version does not use the old SDL2 renderer index or accelerated flag.
    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer)
    {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << '\n';
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Create the board model and the controller.
    // The board stores the game state.
    // The controller handles clicks, selected squares, legal moves, AI updates, and animation state.
    board b;
    GameController controller;

    bool running = true;
    SDL_Event event;

    // Main game loop.
    // This loop continues until the player closes the window.
    while (running)
    {
        // Process all pending SDL events, such as window close or mouse clicks.
        while (SDL_PollEvent(&event))
        {
            // SDL3 renamed the quit event to SDL_EVENT_QUIT.
            if (event.type == SDL_EVENT_QUIT)
            {
                running = false;
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                // Only react to left mouse button clicks.
                if (event.button.button == SDL_BUTTON_LEFT)
                {
                    // SDL3 mouse coordinates are floating-point values.
                    float mouseX = event.button.x;
                    float mouseY = event.button.y;

                    // Convert pixel coordinates into board row/column coordinates.
                    int col = (int)(mouseX / TILE_SIZE);
                    int row = (int)(mouseY / TILE_SIZE);

                    // Let the controller decide what the click means:
                    // selecting a piece, changing selection, or attempting a move.
                    controller.handleClick(b, row, col);
                }
            }
        }

        // Allow the controller to finish or apply any AI move that is ready.
        // This keeps the SDL event loop responsive while the AI works asynchronously.
        controller.updateAI(b);

        // Advance any active movement animation.
        controller.updateAnimation();

        // Clear the screen before drawing the new frame.
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw the frame in layers:
        // 1. board
        // 2. selection highlight
        // 3. legal move markers
        // 4. non-animated pieces
        // 5. animated moving piece
        // 6. AI thinking indicator
        drawCheckerboard(renderer);
        drawSelectedSquare(renderer, controller.selectedRow, controller.selectedCol);
        drawLegalMoves(renderer, controller.legalMoves);
        drawPieces(renderer, b, controller.animation);
        drawMoveAnimation(renderer, controller.animation);
        drawThinkingIndicator(renderer, controller.aiThinking);

        // Present the completed frame to the window.
        SDL_RenderPresent(renderer);
    }

    // Clean up SDL resources before exiting.
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
