/*
 * assetManager.h
 *
 *      Author: alex@glassoniongames.com
 */

#ifndef ASSETMANAGER_H
#define ASSETMANAGER_H

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <string>

class AssetManager
{
public:
    std::string basePath;

    // Core Assets
    TTF_Font *font = nullptr;
    TTF_Font *uiFont = nullptr;

    SDL_Texture *boardTexture = nullptr;
    SDL_Texture *redTexture = nullptr;
    SDL_Texture *blackTexture = nullptr;
    SDL_Texture *redKingTexture = nullptr;
    SDL_Texture *blackKingTexture = nullptr;
    SDL_Texture *legalMoveTexture = nullptr;

    // UI Button Textures
    SDL_Texture *newGameTex = nullptr;
    SDL_Texture *newGameFilledTex = nullptr;
    SDL_Texture *undoTex = nullptr;
    SDL_Texture *undoFilledTex = nullptr;
    SDL_Texture *redoTex = nullptr;
    SDL_Texture *redoFilledTex = nullptr;

    SDL_Texture *onePlayerTex = nullptr;
    SDL_Texture *onePlayerFilledTex = nullptr;
    SDL_Texture *twoPlayerTex = nullptr;
    SDL_Texture *twoPlayerFilledTex = nullptr;
    SDL_Texture *resumeTex = nullptr;
    SDL_Texture *resumeFilledTex = nullptr;
    SDL_Texture *soundOnTex = nullptr;
    SDL_Texture *soundOnFilledTex = nullptr;
    SDL_Texture *soundOffTex = nullptr;
    SDL_Texture *soundOffFilledTex = nullptr;

    SDL_Texture *homePageTex = nullptr;
    SDL_Texture *homePageFilledTex = nullptr;
    SDL_Texture *privacyTex = nullptr;
    SDL_Texture *privacyFilledTex = nullptr;

    SDL_Texture *homeTex = nullptr;
    SDL_Texture *homeFilledTex = nullptr;

    // Audio
    MIX_Audio *moveSfx = nullptr;
    MIX_Audio *captureSfx = nullptr;
    MIX_Audio *winSfx = nullptr;

    std::string getAssetPath(const std::string &relativePath);
    bool loadAssets(SDL_Window *window, SDL_Renderer *renderer, MIX_Mixer *mixer);
    void freeAssets();

private:
    // Procedural Fallback Generators
    SDL_Texture *createBoardTexture(SDL_Renderer *renderer, int size);
    SDL_Texture *createRectTexture(SDL_Renderer *renderer, int w, int h, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
    SDL_Surface *createIconSurface(int size, Uint8 r, Uint8 g, Uint8 b);
    SDL_Texture *createCircleTexture(SDL_Renderer *renderer, int size, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
};

#endif