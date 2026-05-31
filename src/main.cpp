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
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <string>
#include <vector>
#include "board.h"
#include "gameController.h"

struct AppState
{
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;

    std::string basePath;

    // Dynamic layout variables calculated every frame
    float screenW = 0;
    float screenH = 0;
    float tileSize = 0;
    float boardXOffset = 0;
    float boardYOffset = 0;

    // Textures for smooth rendering
    TTF_Font *font = nullptr;
    SDL_Texture *boardTexture = nullptr;
    SDL_Texture *redTexture = nullptr;
    SDL_Texture *blackTexture = nullptr;
    SDL_Texture *redKingTexture = nullptr;
    SDL_Texture *blackKingTexture = nullptr;
    SDL_Texture *legalMoveTexture = nullptr;
    SDL_Texture *newGameTexture = nullptr;
    SDL_Texture *newGamePressedTexture = nullptr;
    bool newGamePressed = false;

    SDL_FRect newGameRect;
    SDL_FRect undoRect;
    SDL_FRect redoRect;

    SDL_Texture *undoTexture = nullptr;
    SDL_Texture *undoPressedTexture = nullptr;
    bool undoPressed = false;

    SDL_Texture *redoTexture = nullptr;
    SDL_Texture *redoPressedTexture = nullptr;
    bool redoPressed = false;

    // Game over textures and state
    SDL_Texture *youWinTexture = nullptr;
    SDL_Texture *aiWinTexture = nullptr;
    int winner = 0; // 0: none, 1: Player (Red), 2: AI (Black)

    std::vector<MoveRecord> history;
    int historyIndex = 0;

    board b;
    GameController controller;
};

// Helper to resolve absolute asset paths, critical for iOS sandboxing.
std::string getAssetPath(const std::string &relativePath, AppState *state)
{
    if (state->basePath.empty())
    {
        const char *base = SDL_GetBasePath();
        if (base)
        {
            std::string baseStr = base;
            SDL_Log("Internal: SDL_GetBasePath reported: %s", baseStr.c_str());
            // In SDL3, the pointer returned by SDL_GetBasePath is internally managed.
            // Do NOT call SDL_free on it, as it will cause a crash during SDL_Quit.

            // Check if the asset exists at the executable's path.
            // If not found, check the parent directory (common for bin/ or build/ folders).
            std::string testPath = baseStr + relativePath;
            SDL_IOStream *io = SDL_IOFromFile(testPath.c_str(), "rb");
            if (!io)
            {
                // Remove trailing slash and find the previous directory separator
                if (baseStr.length() > 1)
                {
                    size_t last = baseStr.find_last_of("\\/", baseStr.length() - 2);
                    if (last != std::string::npos)
                    {
                        std::string parentStr = baseStr.substr(0, last + 1);
                        SDL_IOStream *ioParent = SDL_IOFromFile((parentStr + relativePath).c_str(), "rb");
                        if (ioParent)
                        {
                            baseStr = parentStr;
                            SDL_CloseIO(ioParent);
                        }
                    }
                }
            }
            else
            {
                SDL_CloseIO(io);
            }
            state->basePath = baseStr;
        }
    }
    return state->basePath + relativePath;
}

// Helper to render a string into a texture using SDL_ttf
SDL_Texture *createTextureFromText(SDL_Renderer *renderer, TTF_Font *font, const char *text, SDL_Color color)
{
    if (!font || !text || text[0] == '\0')
        return nullptr;

    // Render the text to a high-quality surface
    SDL_Surface *surface = TTF_RenderText_Blended(font, text, 0, color);
    if (!surface)
        return nullptr;

    // Convert surface to GPU texture
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

    if (texture)
    {
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        // Enable linear scaling so text looks smooth when resized
        SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);
    }

    SDL_DestroySurface(surface);
    return texture;
}

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
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
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
    if (texture)
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_DestroySurface(surface);
    return texture;
}

// Replays the move history from the beginning up to the current historyIndex.
void replayHistory(AppState *state)
{
    state->b.startup();
    for (int i = 0; i < state->historyIndex; ++i)
    {
        const auto &m = state->history[i];
        state->b.tryMove8x8(m.fromRow, m.fromCol, m.toRow, m.toCol);
    }
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

    // Draw captured pieces that are still "waiting" to be visually removed
    for (const auto &cap : state->controller.pendingCaptures)
    {
        if (state->controller.animationPathIndex <= cap.captureStep)
        {
            drawPiece(renderer, cap.row, cap.col, cap.piece, state);
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

    if (!texToDraw)
        return;

    SDL_RenderTexture(renderer, texToDraw, NULL, &state->newGameRect);
}

void drawUndoButton(SDL_Renderer *renderer, AppState *state)
{
    SDL_Texture *texToDraw = state->undoPressed ? state->undoPressedTexture : state->undoTexture;
    if (!texToDraw)
        return;

    SDL_RenderTexture(renderer, texToDraw, NULL, &state->undoRect);
}

void drawRedoButton(SDL_Renderer *renderer, AppState *state)
{
    SDL_Texture *texToDraw = state->redoPressed ? state->redoPressedTexture : state->redoTexture;
    if (!texToDraw)
        return;

    SDL_RenderTexture(renderer, texToDraw, NULL, &state->redoRect);
}

void drawGameOverMessage(SDL_Renderer *renderer, AppState *state)
{
    if (state->winner == 0)
        return;

    SDL_Texture *tex = (state->winner == 1) ? state->youWinTexture : state->aiWinTexture;
    if (!tex)
        return;

    // Position: Centered horizontally, below the New Game button
    float msgW = state->tileSize * 4.0f;
    float msgH = state->tileSize * 1.0f;
    float msgX = state->boardXOffset + (state->tileSize * 8.0f - msgW) / 2.0f;
    // Position: Just below the row of buttons
    float msgY = state->newGameRect.y + state->newGameRect.h + (state->tileSize * 0.2f);

    SDL_FRect dst = {msgX, msgY, msgW, msgH};
    SDL_RenderTexture(renderer, tex, NULL, &dst);
}

// Draws a simple visual indicator while the AI is calculating its move.
void drawThinkingIndicator(SDL_Renderer *renderer, AppState *state)
{
    if (!state->controller.aiThinking)
        return;

    // Enable blending for this draw call so the alpha transparency works
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    SDL_FRect box;
    box.w = state->tileSize * 2.0f;
    box.h = state->tileSize * 0.5f;
    // Center horizontally: (Total Board Width - Box Width) / 2
    box.x = state->boardXOffset + (state->tileSize * 8.0f - box.w) / 2.0f;
    // Position above the board: Top of board - height of box - small margin
    box.y = state->boardYOffset - box.h - (state->tileSize * 0.2f);

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

    // Restore default blend mode
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
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

    // Initialize the font library
    if (!TTF_Init())
    {
        SDL_Log("TTF_Init Error: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Set a logical 400x800 coordinate system.
    // SDL will now scale everything and fix mouse coordinates automatically.
    SDL_SetRenderLogicalPresentation(state->renderer, 400, 800, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    // Load textures using the absolute asset path
    std::string boardPath = getAssetPath("assets/board.png", state);
    state->boardTexture = IMG_LoadTexture(state->renderer, boardPath.c_str());
    if (!state->boardTexture)
    {
        SDL_Log("Warning: Could not load board.png from %s. Error: %s", boardPath.c_str(), SDL_GetError());
        state->boardTexture = createBoardTexture(state->renderer, 1024);
    }
    if (state->boardTexture)
        SDL_SetTextureScaleMode(state->boardTexture, SDL_SCALEMODE_LINEAR);

    // Load PNG textures for the pieces
    state->redTexture = IMG_LoadTexture(state->renderer, getAssetPath("assets/red_piece.png", state).c_str());
    state->blackTexture = IMG_LoadTexture(state->renderer, getAssetPath("assets/black_piece.png", state).c_str());
    state->redKingTexture = IMG_LoadTexture(state->renderer, getAssetPath("assets/red_king.png", state).c_str());
    state->blackKingTexture = IMG_LoadTexture(state->renderer, getAssetPath("assets/black_king.png", state).c_str());

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
    std::string newGamePath = getAssetPath("assets/new_game.png", state);
    state->newGameTexture = IMG_LoadTexture(state->renderer, newGamePath.c_str());
    if (!state->newGameTexture)
    {
        SDL_Log("Warning: Could not load new_game.png from %s", newGamePath.c_str());
        state->newGameTexture = createCircleTexture(state->renderer, 256, 120, 120, 120, 255);
    }

    if (state->newGameTexture)
        SDL_SetTextureScaleMode(state->newGameTexture, SDL_SCALEMODE_LINEAR);

    // Load the pressed version of the New Game button (filled circle symbol)
    state->newGamePressedTexture = IMG_LoadTexture(state->renderer, getAssetPath("assets/new_game_filled.png", state).c_str());
    if (!state->newGamePressedTexture)
    {
        SDL_Log("Warning: Could not load new_game_filled.png, using procedural fallback.");
        state->newGamePressedTexture = createCircleTexture(state->renderer, 256, 200, 200, 200, 255);
    }
    if (state->newGamePressedTexture)
        SDL_SetTextureScaleMode(state->newGamePressedTexture, SDL_SCALEMODE_LINEAR);

    // Load Undo button textures
    state->undoTexture = IMG_LoadTexture(state->renderer, getAssetPath("assets/undo.png", state).c_str());
    if (!state->undoTexture)
    {
        SDL_Log("Warning: Using procedural fallback for Undo button.");
        state->undoTexture = createCircleTexture(state->renderer, 256, 100, 100, 200, 255);
    }
    SDL_SetTextureScaleMode(state->undoTexture, SDL_SCALEMODE_LINEAR);

    state->undoPressedTexture = IMG_LoadTexture(state->renderer, getAssetPath("assets/undo_filled.png", state).c_str());
    if (!state->undoPressedTexture)
    {
        state->undoPressedTexture = createCircleTexture(state->renderer, 256, 150, 150, 255, 255);
    }
    SDL_SetTextureScaleMode(state->undoPressedTexture, SDL_SCALEMODE_LINEAR);

    // Load Redo button textures
    state->redoTexture = IMG_LoadTexture(state->renderer, getAssetPath("assets/redo.png", state).c_str());
    if (!state->redoTexture)
    {
        SDL_Log("Warning: Using procedural fallback for Redo button.");
        state->redoTexture = createCircleTexture(state->renderer, 256, 100, 200, 100, 255);
    }
    SDL_SetTextureScaleMode(state->redoTexture, SDL_SCALEMODE_LINEAR);

    state->redoPressedTexture = IMG_LoadTexture(state->renderer, getAssetPath("assets/redo_filled.png", state).c_str());
    if (!state->redoPressedTexture)
    {
        state->redoPressedTexture = createCircleTexture(state->renderer, 256, 150, 255, 150, 255);
    }
    SDL_SetTextureScaleMode(state->redoPressedTexture, SDL_SCALEMODE_LINEAR);

    // Load a font and generate text textures
    // Note: You must place a .ttf file in your assets folder!
    std::string fontPath = getAssetPath("assets/DayPosterBlackNF.ttf", state);
    state->font = TTF_OpenFont(fontPath.c_str(), 64);
    if (state->font)
    {
        SDL_Color green = {40, 200, 40, 255};
        SDL_Color red = {200, 40, 40, 255};
        state->youWinTexture = createTextureFromText(state->renderer, state->font, "YOU WIN!", green);
        state->aiWinTexture = createTextureFromText(state->renderer, state->font, "AI WINS!", red);
    }
    else
    {
        SDL_Log("Warning: Could not load font from %s. Error: %s", fontPath.c_str(), SDL_GetError());
        state->youWinTexture = createRectTexture(state->renderer, 256, 64, 40, 200, 40, 255);
        state->aiWinTexture = createRectTexture(state->renderer, 256, 64, 200, 40, 40, 255);
    }

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

            bool insideNewGame = (event->button.x >= state->newGameRect.x && event->button.x <= state->newGameRect.x + state->newGameRect.w &&
                                  event->button.y >= state->newGameRect.y && event->button.y <= state->newGameRect.y + state->newGameRect.h);

            bool insideUndo = (event->button.x >= state->undoRect.x && event->button.x <= state->undoRect.x + state->undoRect.w &&
                               event->button.y >= state->undoRect.y && event->button.y <= state->undoRect.y + state->undoRect.h);

            bool insideRedo = (event->button.x >= state->redoRect.x && event->button.x <= state->redoRect.x + state->redoRect.w &&
                               event->button.y >= state->redoRect.y && event->button.y <= state->redoRect.y + state->redoRect.h);

            if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                if (insideNewGame)
                {
                    state->newGamePressed = true;
                }
                else if (insideUndo && !state->controller.aiThinking)
                {
                    state->undoPressed = true;
                }
                else if (insideRedo && !state->controller.aiThinking)
                {
                    state->redoPressed = true;
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
                        state->controller.handleClick(state->b, row, col, state->history, state->historyIndex);
                    }
                }
            }
            else if (event->type == SDL_EVENT_MOUSE_BUTTON_UP)
            {
                if (state->newGamePressed && insideNewGame)
                {
                    // Execute reset only if we released the mouse while still over the button
                    state->b.startup();
                    state->controller.selectedRow = -1;
                    state->controller.selectedCol = -1;
                    state->controller.legalMoves.clear();
                    state->controller.animation.active = false;
                    state->controller.aiMoveReady = false;
                    state->controller.pendingCaptures.clear();
                    state->history.clear();
                    state->historyIndex = 0;
                    state->winner = 0;
                    std::cout << "New Game started!\n";
                }
                state->newGamePressed = false;

                if (state->undoPressed && insideUndo)
                {
                    if (state->historyIndex >= 2)
                    {
                        state->historyIndex -= 2; // Jump back past AI move and Player move
                        replayHistory(state);
                        state->controller.selectedRow = -1;
                        state->controller.selectedCol = -1;
                        state->controller.legalMoves.clear();
                        state->controller.animation.active = false;
                        state->controller.aiMoveReady = false;
                        state->controller.pendingCaptures.clear();
                        state->winner = 0;
                    }
                }
                state->undoPressed = false;

                if (state->redoPressed && insideRedo)
                {
                    if (state->historyIndex + 2 <= (int)state->history.size())
                    {
                        state->historyIndex += 2; // Advance forward past Player move and AI move
                        replayHistory(state);
                        state->controller.selectedRow = -1;
                        state->controller.selectedCol = -1;
                        state->controller.legalMoves.clear();
                        state->controller.animation.active = false;
                        state->controller.aiMoveReady = false;
                        state->controller.pendingCaptures.clear();
                        state->winner = 0;
                    }
                }
                state->redoPressed = false;
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

    // Centralize button layout calculations
    float btnSize = state->tileSize * 1.0f;
    float spacing = state->tileSize * 0.5f;
    float totalWidth = state->tileSize * 8.0f;

    // New Game: Centered at the bottom
    float btnYBottom = state->boardYOffset + (state->tileSize * 8.0f) + (state->tileSize * 0.5f);
    state->newGameRect = {state->boardXOffset + (totalWidth - btnSize) / 2.0f, btnYBottom, btnSize, btnSize};

    // Undo & Redo: Centered at the top, above the thinking indicator
    float btnYTop = state->boardYOffset - btnSize - (state->tileSize * 1.2f);
    float pairWidth = (btnSize * 2.0f) + spacing;
    float startX = state->boardXOffset + (totalWidth - pairWidth) / 2.0f;

    state->undoRect = {startX, btnYTop, btnSize, btnSize};
    state->redoRect = {startX + btnSize + spacing, btnYTop, btnSize, btnSize};

    state->controller.updateAI(state->b, state->history, state->historyIndex);
    state->controller.updateAnimation();

    // Check for win condition if no animation is playing
    if (!state->controller.animation.active && state->winner == 0)
    {
        if (state->b.terminalTest())
        {
            // Current player has no moves
            if (state->b.getTurnPublic() == 'r')
                state->winner = 2; // Red (Human) has no moves, AI wins
            else
                state->winner = 1; // Black (AI) has no moves, Human wins

            std::cout << "Game Over! Winner: " << (state->winner == 1 ? "You" : "AI") << std::endl;
        }
    }

    SDL_SetRenderDrawColor(state->renderer, 0, 0, 0, 255);
    SDL_RenderClear(state->renderer);

    drawCheckerboard(state->renderer, state);
    drawSelectedSquare(state->renderer, state);
    drawLegalMoves(state->renderer, state);
    drawPieces(state->renderer, state);
    drawMoveAnimation(state->renderer, state);
    drawNewGameButton(state->renderer, state);
    drawUndoButton(state->renderer, state);
    drawRedoButton(state->renderer, state);
    drawGameOverMessage(state->renderer, state);
    drawThinkingIndicator(state->renderer, state);

    SDL_RenderPresent(state->renderer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    AppState *state = static_cast<AppState *>(appstate);

    if (state)
    {
        // Safety: If the AI is still thinking in a background thread, we must wait
        // for it to finish before deleting the state, otherwise the detached
        // thread will crash when it tries to access the deleted controller/mutex.
        SDL_Log("Shutting down AI thread...");
        state->controller.stopAI();

        if (state->boardTexture)
            SDL_DestroyTexture(state->boardTexture);
        if (state->redTexture)
            SDL_DestroyTexture(state->redTexture);
        if (state->blackTexture)
            SDL_DestroyTexture(state->blackTexture);

        // Safety: Only destroy king textures if they aren't aliases for the base textures
        if (state->redKingTexture && state->redKingTexture != state->redTexture)
            SDL_DestroyTexture(state->redKingTexture);
        if (state->blackKingTexture && state->blackKingTexture != state->blackTexture)
            SDL_DestroyTexture(state->blackKingTexture);

        if (state->legalMoveTexture)
            SDL_DestroyTexture(state->legalMoveTexture);
        if (state->newGameTexture)
            SDL_DestroyTexture(state->newGameTexture);
        if (state->newGamePressedTexture)
            SDL_DestroyTexture(state->newGamePressedTexture);
        if (state->undoTexture)
            SDL_DestroyTexture(state->undoTexture);
        if (state->undoPressedTexture)
            SDL_DestroyTexture(state->undoPressedTexture);
        if (state->redoTexture)
            SDL_DestroyTexture(state->redoTexture);
        if (state->redoPressedTexture)
            SDL_DestroyTexture(state->redoPressedTexture);
        if (state->youWinTexture)
            SDL_DestroyTexture(state->youWinTexture);
        if (state->aiWinTexture)
            SDL_DestroyTexture(state->aiWinTexture);

        if (state->font)
            TTF_CloseFont(state->font);

        TTF_Quit();

        if (state->renderer)
            SDL_DestroyRenderer(state->renderer);

        if (state->window)
            SDL_DestroyWindow(state->window);

        delete state;
    }
}