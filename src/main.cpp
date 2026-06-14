//=================================================================================
// Name        : Checkr
// Author      : Copyright 2026 Glass Onion Games LLC
// Email       : alex@glassoniongames.com
// Version     : 1.30
// License     : MIT License
// Description : A checkers game GUI frontend.
//=================================================================================

#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <iostream>
#include <string>
#include <vector>
#include "widgets.h"
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
    TTF_Font *uiFont = nullptr;
    SDL_Texture *boardTexture = nullptr;
    SDL_Texture *redTexture = nullptr;
    SDL_Texture *blackTexture = nullptr;
    SDL_Texture *redKingTexture = nullptr;
    SDL_Texture *blackKingTexture = nullptr;
    SDL_Texture *legalMoveTexture = nullptr;

    // SDL3_mixer 3.x uses MIX_Mixer and MIX_Audio instead of Mix_Chunk
    MIX_Mixer *mixer = nullptr;
    MIX_Audio *moveSfx = nullptr;
    MIX_Audio *captureSfx = nullptr;
    MIX_Audio *winSfx = nullptr;

    TextureButton newGameBtn;
    TextureButton undoBtn;
    TextureButton redoBtn;
    TextureButton soundBtn;
    TextureButton homeBtn;
    TextureButton privacyBtn;

    bool soundEnabled = true;
    SDL_Texture *soundOnTex = nullptr;
    SDL_Texture *soundOnFilledTex = nullptr;
    SDL_Texture *soundOffTex = nullptr;
    SDL_Texture *soundOffFilledTex = nullptr;
    SDL_Texture *homeTex = nullptr;
    SDL_Texture *homeFilledTex = nullptr;
    SDL_Texture *privacyTex = nullptr;
    SDL_Texture *privacyFilledTex = nullptr;

    ToggleSwitch *pvpToggle = nullptr;
    Label pvpLabelHuman;
    Label pvpLabelAi;

    // Composition-based Modal
    DialogBox timeModal;
    Label modalTitle;
    RadioButtonGroup difficultyGroup;
    RadioButton modalOptions[6];
    TextureButton modalStartBtn;

    WorkingIndicator workingIndicator;

    // Game over labels and state
    Label redWinLbl;
    Label blackWinLbl;
    int winner = 0; // 0: none, 1: Player Red, 2: Player Black

    Label searchDepthLbl;
    int lastDisplayedDepth = -1;

    std::vector<MoveRecord> history;
    int historyIndex = 0;

    board b;
    GameController controller;
};

// Helper to resolve absolute asset paths, critical for iOS sandboxing.
std::string getAssetPath(const std::string &relativePath, AppState *state)
{
#if defined(SDL_PLATFORM_ANDROID)
    // On Android, assets are often accessed via relative paths from the APK root.
    // SDL3's SDL_IOFromFile handles this automatically if we don't prepend a base path.
    return relativePath;
#else
    // For Desktop/iOS, we maintain the existing discovery logic.
    if (state->basePath.empty())
    {
        const char *base = SDL_GetBasePath();
        if (base)
        {
            std::string baseStr = base;
            SDL_free((void *)base); // SDL3 requires freeing the result of SDL_GetBasePath

            // Try to find the root directory by checking the executable path and up to 2 parent directories.
            // We look for "assets/board.png" as a marker file to confirm we found the correct project root.
            std::string currentCheck = baseStr;
            bool foundRoot = false;

            for (int depth = 0; depth < 3; ++depth)
            {
                std::string markerPath = currentCheck + "assets/board.png";
                SDL_IOStream *io = SDL_IOFromFile(markerPath.c_str(), "rb");
                if (io)
                {
                    state->basePath = currentCheck;
                    SDL_CloseIO(io);
                    foundRoot = true;
                    break;
                }

                // Move up one directory and try again
                if (currentCheck.length() <= 1)
                    break;
                size_t last = currentCheck.find_last_of("\\/", currentCheck.length() - 2);
                if (last == std::string::npos)
                    break;
                currentCheck = currentCheck.substr(0, last + 1);
            }

            // If no marker was found, default back to the absolute executable path
            if (!foundRoot)
            {
                state->basePath = baseStr;
            }
        }
    }
    return state->basePath + relativePath;
#endif
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

// Helper to create a simple fallback icon surface if the PNG is missing.
SDL_Surface *createIconSurface(int size, Uint8 r, Uint8 g, Uint8 b)
{
    SDL_Surface *surface = SDL_CreateSurface(size, size, SDL_PIXELFORMAT_RGBA32);
    if (!surface)
        return nullptr;

    float center = size / 2.0f;
    float radius = (size / 2.0f) - 1.0f;

    for (int y = 0; y < size; y++)
    {
        for (int x = 0; x < size; x++)
        {
            float dx = (float)x + 0.5f - center;
            float dy = (float)y + 0.5f - center;
            float dist = SDL_sqrtf(dx * dx + dy * dy);

            Uint8 *pixel = (Uint8 *)surface->pixels + y * surface->pitch + x * 4;
            pixel[0] = r;
            pixel[1] = g;
            pixel[2] = b;

            if (dist <= radius - 0.5f)
                pixel[3] = 255;
            else if (dist <= radius + 0.5f)
                pixel[3] = (Uint8)(255 * (radius + 0.5f - dist));
            else
                pixel[3] = 0;
        }
    }
    return surface;
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

            if (piece == 'e')
                continue;

            // If this square is part of a pending capture/restore animation, skip drawing
            // it from the board array; we will handle its visibility in the capture loop below.
            if (state->controller.animation.active)
            {
                bool isPending = false;
                for (const auto &cp : state->controller.pendingCaptures)
                {
                    if (cp.row == row && cp.col == col)
                    {
                        isPending = true;
                        break;
                    }
                }
                if (isPending)
                    continue;
            }

            // If a move is currently animating, hide the static piece at its destination.
            // Since tryMove8x8 updates the board state immediately, a multi-jump piece
            // is already at its final square internally even while the animation is
            // still on the first segment. We hide the final destination to prevent "flashing."
            if (state->controller.animation.active)
            {
                int hideRow = state->controller.animation.toRow;
                int hideCol = state->controller.animation.toCol;

                if (!state->controller.animationPath.empty())
                {
                    hideRow = state->controller.animationPath.back().row;
                    hideCol = state->controller.animationPath.back().col;
                }

                if (row == hideRow && col == hideCol)
                    continue;
            }

            drawPiece(renderer, row, col, piece, state);
        }
    }

    // Draw captured pieces that are still "waiting" to be visually removed
    for (const auto &cap : state->controller.pendingCaptures)
    {
        bool shouldDraw = false;
        Uint64 elapsed = SDL_GetTicks() - state->controller.animation.startTime;
        float t = static_cast<float>(elapsed) / static_cast<float>(state->controller.animation.durationMs);

        if (state->controller.animation.isUndo)
        {
            // Undo Logic: Piece is restored. Keep hidden until jumper passes midpoint.
            if (state->controller.animationPathIndex > cap.captureStep)
                shouldDraw = true;
            else if (state->controller.animationPathIndex == cap.captureStep && t >= 0.5f)
                shouldDraw = true;
        }
        else
        {
            // Forward/Redo Logic: Piece is removed. Keep visible until jumper reaches midpoint.
            if (state->controller.animationPathIndex < cap.captureStep)
                shouldDraw = true;
            else if (state->controller.animationPathIndex == cap.captureStep && t < 0.5f)
                shouldDraw = true;
        }

        if (shouldDraw)
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

void drawGameOverMessage(SDL_Renderer *renderer, AppState *state)
{
    if (state->winner == 1)
        state->redWinLbl.render(renderer);
    else if (state->winner == 2)
        state->blackWinLbl.render(renderer);
}

extern "C" SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    SDL_SetAppMetadata("Checkr", "1.30", "com.glassoniongames.checkr");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << '\n';
        return SDL_APP_FAILURE;
    }

    // Enable V-Sync to prevent the CPU/GPU from overworking,
    // which solves the overheating issue on mobile devices.
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

    AppState *state = new AppState();

    state->window = SDL_CreateWindow(
        "Checkr",
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

    // Set the window icon
    std::string iconPath = getAssetPath("assets/icon.png", state);
    SDL_Surface *iconSurface = IMG_Load(iconPath.c_str());

    if (!iconSurface)
    {
        // Fallback: Create a simple red checker piece icon if the asset is missing
        SDL_Log("Warning: icon.png not found. Using procedural fallback icon.");
        iconSurface = createIconSurface(64, 200, 40, 40);
    }

    if (iconSurface)
    {
        SDL_SetWindowIcon(state->window, iconSurface);
        SDL_DestroySurface(iconSurface);
    }

    // Initialize the font library
    if (!TTF_Init())
    {
        SDL_Log("TTF_Init Error: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // SDL3_mixer 3.x Initialization
    if (MIX_Init())
    {
        state->mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
    }
    else
    {
        SDL_Log("MIX_Init Error: %s", SDL_GetError());
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

    // Load Sound Effects using the new MIX_LoadAudio
    if (state->mixer)
    {
        state->moveSfx = MIX_LoadAudio(state->mixer, getAssetPath("assets/move.wav", state).c_str(), false);
        state->captureSfx = MIX_LoadAudio(state->mixer, getAssetPath("assets/capture.wav", state).c_str(), false);
        state->winSfx = MIX_LoadAudio(state->mixer, getAssetPath("assets/win.wav", state).c_str(), false);
    }

    // Load button textures directly into the objects
    // Swapped new_game icons and used 3-arg setup to ensure icons reset on release
    state->newGameBtn.setTextures(
        IMG_LoadTexture(state->renderer, getAssetPath("assets/new_game.png", state).c_str()),
        nullptr,
        IMG_LoadTexture(state->renderer, getAssetPath("assets/new_game_filled.png", state).c_str()));

    state->undoBtn.setTextures(
        IMG_LoadTexture(state->renderer, getAssetPath("assets/undo.png", state).c_str()),
        nullptr,
        IMG_LoadTexture(state->renderer, getAssetPath("assets/undo_filled.png", state).c_str()));

    state->redoBtn.setTextures(
        IMG_LoadTexture(state->renderer, getAssetPath("assets/redo.png", state).c_str()),
        nullptr,
        IMG_LoadTexture(state->renderer, getAssetPath("assets/redo_filled.png", state).c_str()));

    // Load textures for the new utility buttons
    state->soundOnTex = IMG_LoadTexture(state->renderer, getAssetPath("assets/sound_on.png", state).c_str());
    state->soundOnFilledTex = IMG_LoadTexture(state->renderer, getAssetPath("assets/sound_on_filled.png", state).c_str());
    state->soundOffTex = IMG_LoadTexture(state->renderer, getAssetPath("assets/sound_off.png", state).c_str());
    state->soundOffFilledTex = IMG_LoadTexture(state->renderer, getAssetPath("assets/sound_off_filled.png", state).c_str());
    state->homeTex = IMG_LoadTexture(state->renderer, getAssetPath("assets/home.png", state).c_str());
    state->homeFilledTex = IMG_LoadTexture(state->renderer, getAssetPath("assets/home_filled.png", state).c_str());
    state->privacyTex = IMG_LoadTexture(state->renderer, getAssetPath("assets/privacy.png", state).c_str());
    state->privacyFilledTex = IMG_LoadTexture(state->renderer, getAssetPath("assets/privacy_filled.png", state).c_str());

    // Initialize buttons with default textures
    state->soundBtn.setTextures(state->soundOnTex, nullptr, state->soundOnFilledTex);
    state->homeBtn.setTextures(state->homeTex, nullptr, state->homeFilledTex);
    state->privacyBtn.setTextures(state->privacyTex, nullptr, state->privacyFilledTex);

    // Fallback logic could be added here if load() returns false, but current assets are stable.

    // Load a font and generate text textures
    std::string fontPath = getAssetPath("assets/DayPosterBlackNF.ttf", state);
    state->font = TTF_OpenFont(fontPath.c_str(), 64);
    state->uiFont = TTF_OpenFont(fontPath.c_str(), 22);

    if (state->font && state->uiFont)
    {
        // Setup the DialogBox container
        state->timeModal.setColors({20, 20, 20, 240}, {255, 255, 255, 255});
        state->timeModal.setBorderWidth(2.0f);

        // Setup the Human/AI toggle labels
        state->pvpLabelHuman.load(state->renderer, state->uiFont, "Human", {255, 255, 255, 255});
        state->pvpLabelAi.load(state->renderer, state->uiFont, "AI", {255, 255, 255, 255});

        // Initialize PvP Toggle
        state->pvpToggle = new ToggleSwitch({0, 0, 55, 20});
        if (!state->controller.pvpMode)
        {
            state->pvpToggle->setToggled(true);
        }

        state->modalTitle.load(state->renderer, state->uiFont, "Select AI Difficulty", {255, 255, 255, 255});

        int times[] = {3, 5, 10, 20, 30, 60};
        for (int i = 0; i < 6; ++i)
        {
            // Initialize the existing object in the array instead of using '='
            state->modalOptions[i].setValue(times[i]);

            std::string txt = std::to_string(times[i]) + " seconds";
            if (times[i] == 60)
                txt = "1 minute";

            state->modalOptions[i].getLabel().load(state->renderer, state->uiFont, txt, {220, 220, 220, 255});
            state->difficultyGroup.addButton(&state->modalOptions[i]);
        }

        // Use the Group method to set the default selection as you suggested
        state->difficultyGroup.setSelectedValue(3);

        state->timeModal.addChild(&state->difficultyGroup);

        state->modalStartBtn.setTextures(
            IMG_LoadTexture(state->renderer, getAssetPath("assets/OK.png", state).c_str()),
            nullptr,
            IMG_LoadTexture(state->renderer, getAssetPath("assets/OK_filled.png", state).c_str()));

        // Setup Start Button callback
        state->modalStartBtn.setOnClickCallback([state]()
                                                {
            // OFF (False) = Human Mode, ON (True) = AI Mode
            state->controller.pvpMode = (state->pvpToggle ? !state->pvpToggle->isSwitchToggled() : false);
            state->controller.aiTimeLimit = state->difficultyGroup.getSelectedValue();

            SDL_Color white = {255, 255, 255, 255};
            state->blackWinLbl.load(state->renderer, state->font, "BLACK WINS!", white);

            state->b.startup();
            state->timeModal.visible = false;
            state->winner = 0;
            state->history.clear();
            state->historyIndex = 0; });

        state->timeModal.addChild(&state->modalStartBtn);
        state->timeModal.visible = true;

        // 2. Setup Game Over Labels
        SDL_Color white = {255, 255, 255, 255};
        SDL_Color red = {200, 40, 40, 255};
        state->redWinLbl.load(state->renderer, state->font, "RED WINS!", red);
        state->blackWinLbl.load(state->renderer, state->font, "BLACK WINS!", white);
    }
    else
    {
        SDL_Log("Warning: Could not load font from %s. Error: %s", fontPath.c_str(), SDL_GetError());
    }

    *appstate = state;

    return SDL_APP_CONTINUE;
}

extern "C" SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    AppState *state = static_cast<AppState *>(appstate);

    if (event->type == SDL_EVENT_QUIT)
    {
        return SDL_APP_SUCCESS;
    }

    // Convert all pointer events (motion and clicks) to the logical 400x800 coordinate system.
    // This ensures hit detection works correctly on High-DPI (Retina) mobile screens.
    if (event->type == SDL_EVENT_MOUSE_MOTION ||
        event->type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
        event->type == SDL_EVENT_MOUSE_BUTTON_UP)
    {
        SDL_ConvertEventToRenderCoordinates(state->renderer, event);

        // 1. Handle Modal Interaction if visible.
        // The modal blocks all other inputs (buttons and board clicks) until closed.
        if (state->timeModal.visible)
        {
            bool isOverModal = false;
            state->timeModal.handleEvent(event, isOverModal);
            if (state->pvpToggle)
                state->pvpToggle->handleEvent(*event);
            return SDL_APP_CONTINUE;
        }

        // 2. Handle Main Game Buttons
        bool overNew = false, overUndo = false, overRedo = false, overSound = false, overHome = false, overPrivacy = false;

        // Pass all pointer events (including motion for hover effects) to the buttons
        bool clickedNew = state->newGameBtn.handleEvent(event, overNew);
        bool clickedUndo = state->undoBtn.handleEvent(event, overUndo);
        bool clickedRedo = state->redoBtn.handleEvent(event, overRedo);
        bool clickedSound = state->soundBtn.handleEvent(event, overSound);
        bool clickedHome = state->homeBtn.handleEvent(event, overHome);
        bool clickedPrivacy = state->privacyBtn.handleEvent(event, overPrivacy);

        if (clickedNew)
        {
            state->timeModal.visible = true;
        }
        else if (clickedSound)
        {
            state->soundEnabled = !state->soundEnabled;
            if (state->soundEnabled)
                state->soundBtn.setTextures(state->soundOnTex, nullptr, state->soundOnFilledTex);
            else
                state->soundBtn.setTextures(state->soundOffTex, nullptr, state->soundOffFilledTex);
        }
        else if (clickedHome)
        {
#ifndef __EMSCRIPTEN__
            SDL_OpenURL("https://glassoniongames.com");
#endif
        }
        else if (clickedPrivacy)
        {
#ifndef __EMSCRIPTEN__
            SDL_OpenURL("https://glassoniongames.com/privacy-policy/");
#endif
        }
        else if (clickedUndo)
        {
            if (state->historyIndex > 0)
            {
                MoveRecord m = state->history[state->historyIndex - 1];
                state->historyIndex--;
                replayHistory(state);
                char piece = state->b.getPieceAt8x8(m.fromRow, m.fromCol);
                state->controller.setupPathAnimation(state->b, piece, m.fromRow, m.fromCol, m.toRow, m.toCol, true);
                state->controller.selectedRow = -1;
                state->controller.selectedCol = -1;
                state->controller.legalMoves.clear();
                state->winner = 0;
            }
        }
        else if (clickedRedo)
        {
            if (state->historyIndex < (int)state->history.size())
            {
                MoveRecord m = state->history[state->historyIndex];
                char piece = state->b.getPieceAt8x8(m.fromRow, m.fromCol);
                state->controller.setupPathAnimation(state->b, piece, m.fromRow, m.fromCol, m.toRow, m.toCol);
                state->b.tryMove8x8(m.fromRow, m.fromCol, m.toRow, m.toCol);
                state->historyIndex++;
                state->controller.selectedRow = -1;
                state->controller.selectedCol = -1;
                state->controller.legalMoves.clear();
                state->winner = 0;
            }
        }
        else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
        {
            if (event->button.button == SDL_BUTTON_LEFT && !overNew && !overUndo && !overRedo && !overSound && !overHome && !overPrivacy)
            {
                // Only handle board clicks if the click wasn't on a UI button
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
        return SDL_APP_CONTINUE;
    }

    return SDL_APP_CONTINUE;
}

extern "C" SDL_AppResult SDL_AppIterate(void *appstate)
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

    // Update Working Indicator and Modal layouts
    state->workingIndicator.active = state->controller.aiThinking;
    state->workingIndicator.updateLayout(state->boardXOffset + (totalWidth - 100) / 2, state->boardYOffset - 40, 100, 30);

    state->timeModal.updateLayout(40, 200, 320, 400);

    // Update child layouts inside the modal
    float mRectX = state->timeModal.rect.x;
    float mRectY = state->timeModal.rect.y;

    // PvP Toggle layout at the top of the modal
    state->pvpLabelHuman.updateLayout(mRectX + 40, mRectY + 25, 80, 30);
    state->pvpLabelAi.updateLayout(mRectX + 215, mRectY + 25, 30, 30);

    state->pvpToggle->updateLayout(mRectX + 135, mRectY + 30, 55, 20);
    state->pvpToggle->alpha = 1.0f; // Ensure toggle stays visible

    state->modalTitle.updateLayout(mRectX + 20, mRectY + 70, 280, 40);
    state->difficultyGroup.updateLayout(mRectX + 30, mRectY + 130, 25, 25);
    // Move OK button to bottom right
    state->modalStartBtn.updateLayout(mRectX + 320 - btnSize - 20, mRectY + 400 - btnSize - 20, btnSize, btnSize);

    // Disable AI settings and visually grey them out if Human mode is selected
    bool aiSelected = state->pvpToggle ? state->pvpToggle->isSwitchToggled() : true;
    state->modalTitle.alpha = aiSelected ? 1.0f : 0.45f; // Match radio button dimming factor
    state->difficultyGroup.alpha = 1.0f;                 // Let the RadioButton internal logic handle dimming via the enabled flag
    state->difficultyGroup.enabled = aiSelected;

    // Update Game Over Label Layouts
    float msgW = state->tileSize * 4.0f;
    float msgH = state->tileSize * 1.0f;
    float msgX = state->boardXOffset + (state->tileSize * 8.0f - msgW) / 2.0f;
    float msgY = state->newGameBtn.rect.y + state->newGameBtn.rect.h + (state->tileSize * 0.2f);
    state->redWinLbl.updateLayout(msgX, msgY, msgW, msgH);
    state->blackWinLbl.updateLayout(msgX, msgY, msgW, msgH);

    // Update Search Depth Label dynamically (only regenerates texture if value changes)
    int currentDepth = state->controller.currentSearchDepth.load();
    if (currentDepth != state->lastDisplayedDepth)
    {
        state->searchDepthLbl.load(state->renderer, state->uiFont, "Depth: " + std::to_string(currentDepth), {200, 200, 200, 255});
        state->lastDisplayedDepth = currentDepth;
    }
    state->searchDepthLbl.updateLayout(state->boardXOffset + 10, state->boardYOffset - 35, 80, 25);

    // Utility Button Group: New Game, Sound, Home, Privacy
    float bottomBtnSize = state->tileSize * 1.0f;
    float smallSpacing = state->tileSize * 0.3f;
    float largeGap = state->tileSize * 0.9f;
    float bottomGroupWidth = (4.0f * bottomBtnSize) + largeGap + (2.0f * smallSpacing);
    float bottomStartX = (400.0f - bottomGroupWidth) / 2.0f;
    float btnYBottom = state->boardYOffset + (state->tileSize * 8.0f) + (state->tileSize * 0.5f);

    state->newGameBtn.updateLayout(bottomStartX, btnYBottom, bottomBtnSize, bottomBtnSize);
    state->soundBtn.updateLayout(bottomStartX + bottomBtnSize + largeGap, btnYBottom, bottomBtnSize, bottomBtnSize);
    state->homeBtn.updateLayout(bottomStartX + 2.0f * bottomBtnSize + largeGap + smallSpacing, btnYBottom, bottomBtnSize, bottomBtnSize);
    state->privacyBtn.updateLayout(bottomStartX + 3.0f * bottomBtnSize + largeGap + 2.0f * smallSpacing, btnYBottom, bottomBtnSize, bottomBtnSize);

    // Undo & Redo: Centered at the top, above the thinking indicator
    float btnYTop = state->boardYOffset - btnSize - (state->tileSize * 1.2f);
    float pairWidth = (btnSize * 2.0f) + spacing;
    float startX = state->boardXOffset + (totalWidth - pairWidth) / 2.0f;

    state->undoBtn.updateLayout(startX, btnYTop, btnSize, btnSize);
    state->redoBtn.updateLayout(startX + btnSize + spacing, btnYTop, btnSize, btnSize);

    // Logic Check: Can we actually perform these actions?
    bool engineIdle = !state->controller.aiThinking && !state->controller.animation.active;

    state->undoBtn.enabled = engineIdle && (state->historyIndex > 0);
    state->redoBtn.enabled = engineIdle && (state->historyIndex < (int)state->history.size());
    state->newGameBtn.enabled = engineIdle;

    state->controller.updateAI(state->b, state->history, state->historyIndex);
    state->controller.updateAnimation();

    // Check for sound triggers from the animation system
    if (state->controller.soundTrigger > 0)
    {
        if (state->mixer && state->soundEnabled)
        {
            if (state->controller.soundTrigger == 2) // Capture
            {
                MIX_PlayAudio(state->mixer, state->captureSfx);
            }
            else // Standard Move
            {
                MIX_PlayAudio(state->mixer, state->moveSfx);
            }
        }
        state->controller.soundTrigger = 0; // Reset after playing
    }

    // Check for win condition if no animation is playing
    if (!state->controller.animation.active && state->winner == 0)
    {
        if (state->b.terminalTest())
        {
            // Current player has no moves
            if (state->b.getTurnPublic() == 'r')
                state->winner = 2; // Red has no moves, Black wins
            else
                state->winner = 1; // Black has no moves, Red wins

            std::cout << "Game Over! Winner: " << (state->winner == 1 ? "Red" : "Black") << std::endl;

            if (state->mixer && state->winSfx && state->soundEnabled)
                MIX_PlayAudio(state->mixer, state->winSfx);
        }
    }

    SDL_SetRenderDrawColor(state->renderer, 0, 0, 0, 255);
    SDL_RenderClear(state->renderer);

    drawCheckerboard(state->renderer, state);
    drawSelectedSquare(state->renderer, state);
    drawLegalMoves(state->renderer, state);
    drawPieces(state->renderer, state);
    drawMoveAnimation(state->renderer, state);
    state->newGameBtn.render(state->renderer);
    state->undoBtn.render(state->renderer);
    state->redoBtn.render(state->renderer);
    state->soundBtn.render(state->renderer);
    state->homeBtn.render(state->renderer);
    state->privacyBtn.render(state->renderer);
    drawGameOverMessage(state->renderer, state);
    state->searchDepthLbl.render(state->renderer);
    state->workingIndicator.render(state->renderer);
    state->timeModal.render(state->renderer);

    // Render PvP labels AFTER the modal so they aren't dimmed by the background
    if (state->timeModal.visible)
    {
        state->modalTitle.render(state->renderer);
        state->pvpLabelHuman.render(state->renderer);
        state->pvpLabelAi.render(state->renderer);
        if (state->pvpToggle)
            state->pvpToggle->render(state->renderer);
    }

    SDL_RenderPresent(state->renderer);

    return SDL_APP_CONTINUE;
}

extern "C" void SDL_AppQuit(void *appstate, SDL_AppResult result)
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

        if (state->font)
            TTF_CloseFont(state->font);
        if (state->uiFont)
            TTF_CloseFont(state->uiFont);

        if (state->moveSfx)
            MIX_DestroyAudio(state->moveSfx);
        if (state->captureSfx)
            MIX_DestroyAudio(state->captureSfx);
        if (state->winSfx)
            MIX_DestroyAudio(state->winSfx);

        if (state->soundOnTex)
            SDL_DestroyTexture(state->soundOnTex);
        if (state->soundOnFilledTex)
            SDL_DestroyTexture(state->soundOnFilledTex);
        if (state->soundOffTex)
            SDL_DestroyTexture(state->soundOffTex);
        if (state->soundOffFilledTex)
            SDL_DestroyTexture(state->soundOffFilledTex);
        if (state->homeTex)
            SDL_DestroyTexture(state->homeTex);
        if (state->homeFilledTex)
            SDL_DestroyTexture(state->homeFilledTex);
        if (state->privacyTex)
            SDL_DestroyTexture(state->privacyTex);
        if (state->privacyFilledTex)
            SDL_DestroyTexture(state->privacyFilledTex);

        if (state->pvpToggle)
            delete state->pvpToggle;

        if (state->mixer)
            MIX_DestroyMixer(state->mixer);

        MIX_Quit();

        TTF_Quit();

        if (state->renderer)
            SDL_DestroyRenderer(state->renderer);

        if (state->window)
            SDL_DestroyWindow(state->window);

        delete state;
    }
}