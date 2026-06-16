/*
 * appState.h
 *
 *      Author: alex@glassoniongames.com
 */

#ifndef APPSTATE_H
#define APPSTATE_H

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <memory>

#include "assetManager.h"
#include "scene.h"

enum class SceneID
{
    None,
    MainMenu,
    Game
};

struct AppState
{
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;

    float screenW = 0;
    float screenH = 0;

    // Core asset manager
    AssetManager assets;

    MIX_Mixer *mixer = nullptr;
    bool soundEnabled = true;

    // The actively running scene (Menu, Game, etc.)
    std::unique_ptr<Scene> currentScene;

    // Flag for scene transitions
    SceneID nextScene = SceneID::None;
};

#endif