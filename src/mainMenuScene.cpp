#include "mainMenuScene.h"
#include "appState.h"

MainMenuScene::MainMenuScene(AppState *state)
{
    titleLbl.load(state->renderer, state->assets.font, "CHECKR", {255, 255, 255, 255});
    playBtn.setTextures(state->assets.newGameTex, state->assets.newGameTex, state->assets.newGameFilledTex);

    if (state->soundEnabled)
        soundBtn.setTextures(state->assets.soundOnTex, state->assets.soundOnTex, state->assets.soundOnFilledTex);
    else
        soundBtn.setTextures(state->assets.soundOffTex, state->assets.soundOffTex, state->assets.soundOffFilledTex);

    homeBtn.setTextures(state->assets.homeTex, state->assets.homeTex, state->assets.homeFilledTex);
    privacyBtn.setTextures(state->assets.privacyTex, state->assets.privacyTex, state->assets.privacyFilledTex);
}

void MainMenuScene::enter(AppState *state)
{
    // Using logical 400x800 presentation size
    float logicalW = 400.0f;
    float logicalH = 800.0f;

    titleLbl.updateLayout((logicalW - 200.0f) / 2.0f, 150.0f, 200.0f, 60.0f);

    float btnSize = 60.0f;
    playBtn.updateLayout((logicalW - btnSize) / 2.0f, 350.0f, btnSize, btnSize);

    float bottomBtnSize = 50.0f;
    float spacing = 20.0f;
    float totalBottomW = (bottomBtnSize * 3.0f) + (spacing * 2.0f);
    float startX = (logicalW - totalBottomW) / 2.0f;

    soundBtn.updateLayout(startX, logicalH - bottomBtnSize - 40.0f, bottomBtnSize, bottomBtnSize);
    homeBtn.updateLayout(startX + bottomBtnSize + spacing, logicalH - bottomBtnSize - 40.0f, bottomBtnSize, bottomBtnSize);
    privacyBtn.updateLayout(startX + (bottomBtnSize * 2.0f) + (spacing * 2.0f), logicalH - bottomBtnSize - 40.0f, bottomBtnSize, bottomBtnSize);
}

void MainMenuScene::handleEvent(AppState *state, SDL_Event *event)
{
    bool dummy = false;
    if (playBtn.handleEvent(event, dummy))
    {
        state->nextScene = SceneID::Game;
    }
    else if (soundBtn.handleEvent(event, dummy))
    {
        state->soundEnabled = !state->soundEnabled;
        if (state->soundEnabled)
            soundBtn.setTextures(state->assets.soundOnTex, state->assets.soundOnTex, state->assets.soundOnFilledTex);
        else
            soundBtn.setTextures(state->assets.soundOffTex, state->assets.soundOffTex, state->assets.soundOffFilledTex);
    }
    else if (homeBtn.handleEvent(event, dummy))
    {
#ifndef __EMSCRIPTEN__
        SDL_OpenURL("https://glassoniongames.com");
#endif
    }
    else if (privacyBtn.handleEvent(event, dummy))
    {
#ifndef __EMSCRIPTEN__
        SDL_OpenURL("https://glassoniongames.com/privacy-policy/");
#endif
    }
}

void MainMenuScene::update(AppState *state) {}

void MainMenuScene::render(AppState *state)
{
    SDL_SetRenderDrawColor(state->renderer, 0, 0, 0, 255);
    SDL_RenderClear(state->renderer);

    titleLbl.render(state->renderer);
    playBtn.render(state->renderer);
    soundBtn.render(state->renderer);
    homeBtn.render(state->renderer);
    privacyBtn.render(state->renderer);

    SDL_RenderPresent(state->renderer);
}