/*
 * assetManager.cpp
 *
 *      Author: alex@glassoniongames.com
 */

#include "assetManager.h"

std::string AssetManager::getAssetPath(const std::string &relativePath)
{
#if defined(SDL_PLATFORM_ANDROID)
    return relativePath;
#else
    if (basePath.empty())
    {
        const char *base = SDL_GetBasePath();
        if (base)
        {
            std::string baseStr = base;
            std::string currentCheck = baseStr;
            bool foundRoot = false;

            for (int depth = 0; depth < 3; ++depth)
            {
                std::string markerPath = currentCheck + "assets/board.png";
                SDL_IOStream *io = SDL_IOFromFile(markerPath.c_str(), "rb");
                if (io)
                {
                    basePath = currentCheck;
                    SDL_CloseIO(io);
                    foundRoot = true;
                    break;
                }
                if (currentCheck.length() <= 1)
                    break;
                size_t last = currentCheck.find_last_of("\\/", currentCheck.length() - 2);
                if (last == std::string::npos)
                    break;
                currentCheck = currentCheck.substr(0, last + 1);
            }
            if (!foundRoot)
            {
                basePath = baseStr;
            }
        }
    }
    return basePath + relativePath;
#endif
}

bool AssetManager::loadAssets(SDL_Window *window, SDL_Renderer *renderer, MIX_Mixer *mixer)
{
    bool success = true;

    // Set window icon
    std::string iconPath = getAssetPath("assets/icon.png");
    SDL_Surface *iconSurface = IMG_Load(iconPath.c_str());
    if (!iconSurface)
    {
        SDL_Log("Warning: icon.png not found. Using procedural fallback icon.");
        iconSurface = createIconSurface(64, 200, 40, 40);
    }
    if (iconSurface)
    {
        SDL_SetWindowIcon(window, iconSurface);
        SDL_DestroySurface(iconSurface);
    }

    // Load board
    std::string boardPath = getAssetPath("assets/board.png");
    boardTexture = IMG_LoadTexture(renderer, boardPath.c_str());
    if (!boardTexture)
    {
        SDL_Log("Warning: Could not load board.png from %s. Error: %s", boardPath.c_str(), SDL_GetError());
        boardTexture = createBoardTexture(renderer, 1024);
    }
    if (boardTexture)
        SDL_SetTextureScaleMode(boardTexture, SDL_SCALEMODE_LINEAR);

    // Load Pieces
    redTexture = IMG_LoadTexture(renderer, getAssetPath("assets/red_piece.png").c_str());
    blackTexture = IMG_LoadTexture(renderer, getAssetPath("assets/black_piece.png").c_str());
    redKingTexture = IMG_LoadTexture(renderer, getAssetPath("assets/red_king.png").c_str());
    blackKingTexture = IMG_LoadTexture(renderer, getAssetPath("assets/black_king.png").c_str());

    if (redTexture)
        SDL_SetTextureScaleMode(redTexture, SDL_SCALEMODE_LINEAR);
    if (blackTexture)
        SDL_SetTextureScaleMode(blackTexture, SDL_SCALEMODE_LINEAR);
    if (redKingTexture)
        SDL_SetTextureScaleMode(redKingTexture, SDL_SCALEMODE_LINEAR);
    if (blackKingTexture)
        SDL_SetTextureScaleMode(blackKingTexture, SDL_SCALEMODE_LINEAR);

    if (!redTexture)
        redTexture = createCircleTexture(renderer, 256, 200, 40, 40, 255);
    if (!redKingTexture)
        redKingTexture = redTexture;
    if (!blackTexture)
        blackTexture = createCircleTexture(renderer, 256, 20, 20, 20, 255);
    if (!blackKingTexture)
        blackKingTexture = blackTexture;

    legalMoveTexture = createCircleTexture(renderer, 256, 0, 0, 255, 180);
    if (legalMoveTexture)
        SDL_SetTextureScaleMode(legalMoveTexture, SDL_SCALEMODE_LINEAR);

    // Load Audio
    if (mixer)
    {
        moveSfx = MIX_LoadAudio(mixer, getAssetPath("assets/move.wav").c_str(), false);
        captureSfx = MIX_LoadAudio(mixer, getAssetPath("assets/capture.wav").c_str(), false);
        winSfx = MIX_LoadAudio(mixer, getAssetPath("assets/win.wav").c_str(), false);
    }

    // Load UI Elements
    auto loadUITex = [&](const std::string &path, Uint8 r, Uint8 g, Uint8 b) -> SDL_Texture *
    {
        SDL_Texture *t = IMG_LoadTexture(renderer, getAssetPath(path).c_str());
        if (!t)
        {
            SDL_Log("Warning: Missing UI asset %s. Using procedural fallback.", path.c_str());
            t = createRectTexture(renderer, 50, 50, r, g, b, 255);
        }
        return t;
    };

    newGameTex = loadUITex("assets/new_game.png", 100, 100, 100);
    newGameFilledTex = loadUITex("assets/new_game_filled.png", 150, 150, 150);
    undoTex = loadUITex("assets/undo.png", 100, 100, 100);
    undoFilledTex = loadUITex("assets/undo_filled.png", 150, 150, 150);
    redoTex = loadUITex("assets/redo.png", 100, 100, 100);
    redoFilledTex = loadUITex("assets/redo_filled.png", 150, 150, 150);
    okTex = loadUITex("assets/OK.png", 100, 100, 100);
    okFilledTex = loadUITex("assets/OK_filled.png", 150, 150, 150);

    soundOnTex = loadUITex("assets/sound_on.png", 100, 100, 100);
    soundOnFilledTex = loadUITex("assets/sound_on_filled.png", 150, 150, 150);
    soundOffTex = loadUITex("assets/sound_off.png", 100, 100, 100);
    soundOffFilledTex = loadUITex("assets/sound_off_filled.png", 150, 150, 150);
    homeTex = loadUITex("assets/home.png", 100, 100, 100);
    homeFilledTex = loadUITex("assets/home_filled.png", 150, 150, 150);
    privacyTex = loadUITex("assets/privacy.png", 100, 100, 100);
    privacyFilledTex = loadUITex("assets/privacy_filled.png", 150, 150, 150);

    // Load Fonts
    std::string fontPath = getAssetPath("assets/DayPosterBlackNF.ttf");
    font = TTF_OpenFont(fontPath.c_str(), 80);   // Increased for larger, crisper win messages
    uiFont = TTF_OpenFont(fontPath.c_str(), 32); // Increased significantly for readable standard UI text

    if (!font || !uiFont)
    {
        SDL_Log("Warning: Could not load font from %s. Error: %s", fontPath.c_str(), SDL_GetError());
        success = false;
    }

    return success;
}

void AssetManager::freeAssets()
{
    if (boardTexture)
        SDL_DestroyTexture(boardTexture);
    if (redTexture)
        SDL_DestroyTexture(redTexture);
    if (blackTexture)
        SDL_DestroyTexture(blackTexture);

    if (redKingTexture && redKingTexture != redTexture)
        SDL_DestroyTexture(redKingTexture);
    if (blackKingTexture && blackKingTexture != blackTexture)
        SDL_DestroyTexture(blackKingTexture);

    if (legalMoveTexture)
        SDL_DestroyTexture(legalMoveTexture);

    if (newGameTex)
        SDL_DestroyTexture(newGameTex);
    if (newGameFilledTex)
        SDL_DestroyTexture(newGameFilledTex);
    if (undoTex)
        SDL_DestroyTexture(undoTex);
    if (undoFilledTex)
        SDL_DestroyTexture(undoFilledTex);
    if (redoTex)
        SDL_DestroyTexture(redoTex);
    if (redoFilledTex)
        SDL_DestroyTexture(redoFilledTex);
    if (okTex)
        SDL_DestroyTexture(okTex);
    if (okFilledTex)
        SDL_DestroyTexture(okFilledTex);

    if (soundOnTex)
        SDL_DestroyTexture(soundOnTex);
    if (soundOnFilledTex)
        SDL_DestroyTexture(soundOnFilledTex);
    if (soundOffTex)
        SDL_DestroyTexture(soundOffTex);
    if (soundOffFilledTex)
        SDL_DestroyTexture(soundOffFilledTex);
    if (homeTex)
        SDL_DestroyTexture(homeTex);
    if (homeFilledTex)
        SDL_DestroyTexture(homeFilledTex);
    if (privacyTex)
        SDL_DestroyTexture(privacyTex);
    if (privacyFilledTex)
        SDL_DestroyTexture(privacyFilledTex);

    if (font)
        TTF_CloseFont(font);
    if (uiFont)
        TTF_CloseFont(uiFont);

    if (moveSfx)
        MIX_DestroyAudio(moveSfx);
    if (captureSfx)
        MIX_DestroyAudio(captureSfx);
    if (winSfx)
        MIX_DestroyAudio(winSfx);
}

// --- Procedural Generators ---

SDL_Texture *AssetManager::createBoardTexture(SDL_Renderer *renderer, int size)
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

SDL_Texture *AssetManager::createRectTexture(SDL_Renderer *renderer, int w, int h, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
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

SDL_Surface *AssetManager::createIconSurface(int size, Uint8 r, Uint8 g, Uint8 b)
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

SDL_Texture *AssetManager::createCircleTexture(SDL_Renderer *renderer, int size, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    SDL_Surface *surface = createIconSurface(size, r, g, b);
    if (!surface)
        return nullptr;
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_DestroySurface(surface);
    return texture;
}