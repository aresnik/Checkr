//=================================================================================
// Name        : Checkers
// Author      : Copyright 2026 Alexander Resnik
// Email       : alex@alexanderresnik.com
// Version     : 3.0
// License     : MIT License
// Description : IceCream checkers engine and GUI.
//=================================================================================

#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <iostream>
#include <vector>
#include "board.h"
#include "gameController.h"

struct AppState
{
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;

    // Dynamic layout variables calculated every frame
    float screenW = 0;
    float screenH = 0;
    float tileSize = 0;
    float boardXOffset = 0;
    float boardYOffset = 0;

    // Textures for smooth rendering
    SDL_Texture *boardTexture = nullptr;
    SDL_Texture *redTexture = nullptr;
    SDL_Texture *blackTexture = nullptr;
    SDL_Texture *redKingTexture = nullptr;
    SDL_Texture *blackKingTexture = nullptr;
    SDL_Texture *legalMoveTexture = nullptr;
    SDL_Texture *newGameTexture = nullptr;
    SDL_Texture *newGamePressedTexture = nullptr;
    bool newGamePressed = false;

    board b;
    GameController controller;
};

// Helper to create the board background as a single texture.
SDL_Texture *createBoardTexture(SDL_Renderer *renderer, int size)
{
    SDL_Surface *surface = SDL_CreateSurface(size, size, SDL_PIXELFORMAT_RGBA32);
    if (!surface)
        return nullptr;

    const SDL_PixelFormatDetails *details = SDL_GetPixelFormatDetails(surface->format);
    Uint32 darkColor = SDL_MapRGBA(details, NULL, 139, 69, 19, 255);
    Uint32 lightColor = SDL_MapRGBA(details, NULL, 245, 222, 179, 255);

    int tileSize = size / 8;
    for (int row = 0; row < 8; ++row)
    {
        for (int col = 0; col < 8; ++col)
        {
            SDL_Rect rect = {col * tileSize, row * tileSize, tileSize, tileSize};
            bool isDarkSquare = ((row + col) % 2 == 1);
            SDL_FillSurfaceRect(surface, &rect, isDarkSquare ? darkColor : lightColor);
        }
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture)
    {
        SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);
    }
    SDL_DestroySurface(surface);
    return texture;
}

// Helper to create a solid rectangular texture for UI elements.
SDL_Texture *createRectTexture(SDL_Renderer *renderer, int w, int h, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    SDL_Surface *surface = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA32);
    if (!surface)
        return nullptr;

    const SDL_PixelFormatDetails *details = SDL_GetPixelFormatDetails(surface->format);
    Uint32 color = SDL_MapRGBA(details, NULL, r, g, b, a);
    SDL_FillSurfaceRect(surface, NULL, color);

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    return texture;
}

// Helper to create a smooth, anti-aliased circle texture at startup.
SDL_Texture *createCircleTexture(SDL_Renderer *renderer, int size, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    // Create a software surface with an alpha channel
    SDL_Surface *surface = SDL_CreateSurface(size, size, SDL_PIXELFORMAT_RGBA32);
    if (!surface)
        return nullptr;

    float center = size / 2.0f;
    float radius = (size / 2.0f) - 1.0f;

    for (int y = 0; y < size; y++)
    {
        for (int x = 0; x < size; x++)
        {
            // Calculate distance from center to pixel midpoint
            float dx = (float)x + 0.5f - center;
            float dy = (float)y + 0.5f - center;
            float dist = SDL_sqrtf(dx * dx + dy * dy);

            Uint8 *pixel = (Uint8 *)surface->pixels + y * surface->pitch + x * 4;
            pixel[0] = r; // R
            pixel[1] = g; // G
            pixel[2] = b; // B

            if (dist <= radius - 1.0f)
            {
                pixel[3] = a; // Fully opaque inside
            }
            else if (dist <= radius)
            {
                float alphaFactor = radius - dist; // Smooth edge fade
                pixel[3] = (Uint8)(a * alphaFactor);
            }
            else
            {
                pixel[3] = 0; // Transparent outside
            }
        }
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_DestroySurface(surface);
    return texture;
}

// Draws the 8x8 checkerboard background.
void drawCheckerboard(SDL_Renderer *renderer, AppState *state)
{
    float boardDim = state->tileSize * 8.0f;
    SDL_FRect dst = {state->boardXOffset, state->boardYOffset, boardDim, boardDim};
    SDL_RenderTexture(renderer, state->boardTexture, NULL, &dst);
}

// Draws one checker piece at an exact pixel position.
void drawPieceAtPixel(SDL_Renderer *renderer, float centerX, float centerY, char piece, AppState *state)
{
    float radius = state->tileSize * 0.40f;
    SDL_Texture *tex = nullptr;

    if (piece == 'b')
        tex = state->blackTexture;
    else if (piece == 'r')
        tex = state->redTexture;
    else if (piece == 'R')
        tex = state->redKingTexture;
    else if (piece == 'B')
        tex = state->blackKingTexture;

    if (tex)
    {
        SDL_FRect dst = {centerX - radius, centerY - radius, radius * 2.0f, radius * 2.0f};
        SDL_RenderTexture(renderer, tex, NULL, &dst);
    }
}

// Converts a board row/column into the pixel center of that square,
void drawPiece(SDL_Renderer *renderer, int row, int col, char piece, AppState *state)
{
    float centerX = col * state->tileSize + state->tileSize / 2.0f + state->boardXOffset;
    float centerY = row * state->tileSize + state->tileSize / 2.0f + state->boardYOffset;
    drawPieceAtPixel(renderer, centerX, centerY, piece, state);
}

// Draws all pieces from the current board state.
void drawPieces(SDL_Renderer *renderer, AppState *state)
{
    for (int row = 0; row < 8; ++row)
    {
        for (int col = 0; col < 8; ++col)
        {
            char piece = state->b.getPieceAt8x8(row, col);

            if (piece == 'e' || piece == 'x')
                continue;

            if (state->controller.animation.active &&
                row == state->controller.animation.toRow &&
                col == state->controller.animation.toCol)
            {
                continue;
            }

            drawPiece(renderer, row, col, piece, state);
        }
    }
}

void drawSelectedSquare(SDL_Renderer *renderer, AppState *state)
{
    if (state->controller.selectedRow < 0 || state->controller.selectedCol < 0)
        return;

    SDL_FRect highlight;
    highlight.x = state->controller.selectedCol * state->tileSize + state->boardXOffset;
    highlight.y = state->controller.selectedRow * state->tileSize + state->boardYOffset;
    highlight.w = state->tileSize;
    highlight.h = state->tileSize;

    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderRect(renderer, &highlight);
}

// Draws blue dots on squares where the selected piece is allowed to move.
void drawLegalMoves(SDL_Renderer *renderer, AppState *state)
{
    float radius = state->tileSize * 0.15f;

    for (const auto &move : state->controller.legalMoves)
    {
        float centerX = move.col * state->tileSize + state->tileSize / 2.0f + state->boardXOffset;
        float centerY = move.row * state->tileSize + state->tileSize / 2.0f + state->boardYOffset;

        SDL_FRect dst = {centerX - radius, centerY - radius, radius * 2.0f, radius * 2.0f};
        SDL_RenderTexture(renderer, state->legalMoveTexture, NULL, &dst);
    }
}

// Draws the currently moving piece between its starting square and ending square.
void drawMoveAnimation(SDL_Renderer *renderer, AppState *state)
{
    MoveAnimation &animation = state->controller.animation;
    if (!animation.active)
        return;

    Uint64 now = SDL_GetTicks();
    Uint64 elapsed = now - animation.startTime;

    if (elapsed >= (Uint64)animation.durationMs)
    {
        animation.active = false;
        return;
    }

    float t = static_cast<float>(elapsed) / static_cast<float>(animation.durationMs);

    float fromX = animation.fromCol * state->tileSize + state->tileSize / 2.0f + state->boardXOffset;
    float fromY = animation.fromRow * state->tileSize + state->tileSize / 2.0f + state->boardYOffset;
    float toX = animation.toCol * state->tileSize + state->tileSize / 2.0f + state->boardXOffset;
    float toY = animation.toRow * state->tileSize + state->tileSize / 2.0f + state->boardYOffset;

    float currentX = fromX + (toX - fromX) * t;
    float currentY = fromY + (toY - fromY) * t;

    drawPieceAtPixel(renderer, currentX, currentY, animation.piece, state);
}

void drawNewGameButton(SDL_Renderer *renderer, AppState *state)
{
    SDL_Texture *texToDraw = state->newGamePressed ? state->newGamePressedTexture : state->newGameTexture;
    
    if (!texToDraw) return;

    // Place the button below the board, centered horizontally
    float btnW = state->tileSize * 1.0f;
    float btnH = state->tileSize * 1.0f;
    float btnX = state->boardXOffset + (state->tileSize * 8.0f - btnW) / 2.0f;
    // Offset it slightly below the bottom edge of the board
    float btnY = state->boardYOffset + (state->tileSize * 8.0f) + (state->tileSize * 0.5f);

    SDL_FRect dst = {btnX, btnY, btnW, btnH};
    SDL_RenderTexture(renderer, texToDraw, NULL, &dst);
}

// Draws a simple visual indicator while the AI is calculating its move.
void drawThinkingIndicator(SDL_Renderer *renderer, AppState *state)
{
    if (!state->controller.aiThinking)
        return;

    SDL_FRect box;
    box.x = state->boardXOffset + state->tileSize * 0.2f;
    box.y = state->boardYOffset + state->tileSize * 0.2f;
    box.w = state->tileSize * 2.0f;
    box.h = state->tileSize * 0.5f;

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_RenderFillRect(renderer, &box);

    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderRect(renderer, &box);

    Uint32 ticks = SDL_GetTicks();
    int dotCount = (ticks / 300) % 7;

    for (int i = 0; i < dotCount; i++)
    {
        SDL_FRect dot;
        dot.x = box.x + 10 + i * 12;
        dot.y = box.y + (box.h / 2) - 2;
        dot.w = 4;
        dot.h = 4;

        SDL_RenderFillRect(renderer, &dot);
    }
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    SDL_SetAppMetadata("Checkers", "3.0", "com.alexresnik.checkers");

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << '\n';
        return SDL_APP_FAILURE;
    }

    AppState *state = new AppState();

    state->window = SDL_CreateWindow(
        "Checkers",
        400,
        800,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);

    if (!state->window)
    {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << '\n';
        delete state;
        return SDL_APP_FAILURE;
    }

    state->renderer = SDL_CreateRenderer(state->window, nullptr);

    if (!state->renderer)
    {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << '\n';
        SDL_DestroyWindow(state->window);
        delete state;
        return SDL_APP_FAILURE;
    }

    // Set a logical 400x800 coordinate system.
    // SDL will now scale everything and fix mouse coordinates automatically.
    SDL_SetRenderLogicalPresentation(state->renderer, 400, 800, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    // Load PNG texture for the board
    state->boardTexture = IMG_LoadTexture(state->renderer, "assets/board.png");
    if (!state->boardTexture)
    {
        SDL_Log("Warning: Could not load board.png, using procedural fallback.");
        state->boardTexture = createBoardTexture(state->renderer, 1024);
    }
    if (state->boardTexture)
        SDL_SetTextureScaleMode(state->boardTexture, SDL_SCALEMODE_LINEAR);

    // Load PNG textures for the pieces
    state->redTexture = IMG_LoadTexture(state->renderer, "assets/red_piece.png");
    state->blackTexture = IMG_LoadTexture(state->renderer, "assets/black_piece.png");
    state->redKingTexture = IMG_LoadTexture(state->renderer, "assets/red_king.png");
    state->blackKingTexture = IMG_LoadTexture(state->renderer, "assets/black_king.png");

    // Set linear scaling for all piece textures to ensure smooth edges when resized
    if (state->redTexture)
        SDL_SetTextureScaleMode(state->redTexture, SDL_SCALEMODE_LINEAR);
    if (state->blackTexture)
        SDL_SetTextureScaleMode(state->blackTexture, SDL_SCALEMODE_LINEAR);
    if (state->redKingTexture)
        SDL_SetTextureScaleMode(state->redKingTexture, SDL_SCALEMODE_LINEAR);
    if (state->blackKingTexture)
        SDL_SetTextureScaleMode(state->blackKingTexture, SDL_SCALEMODE_LINEAR);

    // Fallback: If images are missing, generate the procedural circles so the game still runs
    if (!state->redTexture)
        state->redTexture = createCircleTexture(state->renderer, 256, 200, 40, 40, 255);
    if (!state->redKingTexture)
        state->redKingTexture = state->redTexture; // Default to regular piece if King asset is missing

    if (!state->blackTexture)
        state->blackTexture = createCircleTexture(state->renderer, 256, 20, 20, 20, 255);
    if (!state->blackKingTexture)
        state->blackKingTexture = state->blackTexture; // Default to regular piece if King asset is missing

    // Keep the procedural texture for legal move indicators
    state->legalMoveTexture = createCircleTexture(state->renderer, 256, 0, 0, 255, 180);
    if (state->legalMoveTexture)
        SDL_SetTextureScaleMode(state->legalMoveTexture, SDL_SCALEMODE_LINEAR);

    // Load the New Game button texture
    state->newGameTexture = IMG_LoadTexture(state->renderer, "assets/new_game.png");
    if (!state->newGameTexture)
    {
        SDL_Log("Warning: Could not load new_game.png, using procedural fallback.");
        state->newGameTexture = createCircleTexture(state->renderer, 256, 120, 120, 120, 255);
    }

    if (state->newGameTexture)
        SDL_SetTextureScaleMode(state->newGameTexture, SDL_SCALEMODE_LINEAR);

    // Load the pressed version of the New Game button (filled circle symbol)
    state->newGamePressedTexture = IMG_LoadTexture(state->renderer, "assets/new_game_filled.png");
    if (!state->newGamePressedTexture)
    {
        SDL_Log("Warning: Could not load new_game_filled.png, using procedural fallback.");
        state->newGamePressedTexture = createCircleTexture(state->renderer, 256, 200, 200, 200, 255);
    }
    if (state->newGamePressedTexture)
        SDL_SetTextureScaleMode(state->newGamePressedTexture, SDL_SCALEMODE_LINEAR);

    *appstate = state;

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    AppState *state = static_cast<AppState *>(appstate);

    if (event->type == SDL_EVENT_QUIT)
    {
        return SDL_APP_SUCCESS;
    }

    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN || event->type == SDL_EVENT_MOUSE_BUTTON_UP)
    {
        if (event->button.button == SDL_BUTTON_LEFT)
        {
            SDL_ConvertEventToRenderCoordinates(state->renderer, event);

            float btnW = state->tileSize * 1.0f;
            float btnH = state->tileSize * 1.0f;
            float btnX = state->boardXOffset + (state->tileSize * 8.0f - btnW) / 2.0f;
            float btnY = state->boardYOffset + (state->tileSize * 8.0f) + (state->tileSize * 0.5f);

            bool insideButton = (event->button.x >= btnX && event->button.x <= btnX + btnW &&
                                 event->button.y >= btnY && event->button.y <= btnY + btnH);

            if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                if (insideButton)
                {
                    state->newGamePressed = true;
                }
                else
                {
                    // Handle board clicks only on DOWN
                    float adjustedX = event->button.x - state->boardXOffset;
                    float adjustedY = event->button.y - state->boardYOffset;
                    int col = static_cast<int>(adjustedX / state->tileSize);
                    int row = static_cast<int>(adjustedY / state->tileSize);

                    if (adjustedX >= 0 && adjustedY >= 0 && col < 8 && row < 8)
                    {
                        state->controller.handleClick(state->b, row, col);
                    }
                }
            }
            else if (event->type == SDL_EVENT_MOUSE_BUTTON_UP)
            {
                if (state->newGamePressed && insideButton)
                {
                    // Execute reset only if we released the mouse while still over the button
                    state->b.startup();
                    state->controller.selectedRow = -1;
                    state->controller.selectedCol = -1;
                    state->controller.legalMoves.clear();
                    state->controller.animation.active = false;
                    state->controller.aiMoveReady = false;
                    std::cout << "New Game started!\n";
                }
                state->newGamePressed = false;
            }
        }
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    AppState *state = static_cast<AppState *>(appstate);

    // With logical presentation (400x800), layout is now constant and simplified.
    // The board is 400x400, centered vertically in the 800 height.
    state->tileSize = 400.0f / 8.0f; // 50.0f
    state->boardXOffset = 0.0f;
    state->boardYOffset = 200.0f; // (800 total height - 400 board height) / 2

    state->controller.updateAI(state->b);
    state->controller.updateAnimation();

    SDL_SetRenderDrawColor(state->renderer, 0, 0, 0, 255);
    SDL_RenderClear(state->renderer);

    drawCheckerboard(state->renderer, state);
    drawSelectedSquare(state->renderer, state);
    drawLegalMoves(state->renderer, state);
    drawPieces(state->renderer, state);
    drawMoveAnimation(state->renderer, state);
    drawNewGameButton(state->renderer, state);
    drawThinkingIndicator(state->renderer, state);

    SDL_RenderPresent(state->renderer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    AppState *state = static_cast<AppState *>(appstate);

    if (state)
    {
        if (state->boardTexture)
            SDL_DestroyTexture(state->boardTexture);
        if (state->redTexture)
            SDL_DestroyTexture(state->redTexture);
        if (state->blackTexture)
            SDL_DestroyTexture(state->blackTexture);
        if (state->redKingTexture)
            SDL_DestroyTexture(state->redKingTexture);
        if (state->blackKingTexture)
            SDL_DestroyTexture(state->blackKingTexture);
        if (state->legalMoveTexture)
            SDL_DestroyTexture(state->legalMoveTexture);
        if (state->newGameTexture)
            SDL_DestroyTexture(state->newGameTexture);
        if (state->newGamePressedTexture)
            SDL_DestroyTexture(state->newGamePressedTexture);

        if (state->renderer)
            SDL_DestroyRenderer(state->renderer);

        if (state->window)
            SDL_DestroyWindow(state->window);

        delete state;
    }

    SDL_Quit();
}