/*
 * mainMenuScene.cpp
 *
 *      Author: alex@glassoniongames.com
 */

#include "mainMenuScene.h"
#include "appState.h"
#include <algorithm>

MainMenuScene::MainMenuScene(AppState *state)
{
    titleLbl.load(state->renderer, state->assets.font, "CHECKR", {255, 255, 255, 255});
    titleLbl.setAlignment(Label::ALIGN_CENTER);
    playBtn.setTextures(state->assets.newGameTex, state->assets.newGameTex, state->assets.newGameFilledTex);

    if (state->soundEnabled)
        soundBtn.setTextures(state->assets.soundOnTex, state->assets.soundOnTex, state->assets.soundOnFilledTex);
    else
        soundBtn.setTextures(state->assets.soundOffTex, state->assets.soundOffTex, state->assets.soundOffFilledTex);

    homeBtn.setTextures(state->assets.homeTex, state->assets.homeTex, state->assets.homeFilledTex);
    privacyBtn.setTextures(state->assets.privacyTex, state->assets.privacyTex, state->assets.privacyFilledTex);

    // Hook up button events via callbacks
    playBtn.setOnClickCallback([state]()
                               { state->nextScene = SceneID::Game; });

    soundBtn.setOnClickCallback([this, state]()
                                {
        state->soundEnabled = !state->soundEnabled;
        if (state->soundEnabled)
            soundBtn.setTextures(state->assets.soundOnTex, state->assets.soundOnTex, state->assets.soundOnFilledTex);
        else
            soundBtn.setTextures(state->assets.soundOffTex, state->assets.soundOffTex, state->assets.soundOffFilledTex); });

    homeBtn.setOnClickCallback([]()
                               {
#ifndef __EMSCRIPTEN__
                                   SDL_OpenURL("https://glassoniongames.com");
#endif
                               });

    privacyBtn.setOnClickCallback([]()
                                  {
#ifndef __EMSCRIPTEN__
                                      SDL_OpenURL("https://glassoniongames.com/privacy-policy/");
#endif
                                  });

    // Build the UI tree
    bottomMenuBox.addChild(&soundBtn, 1.0f);
    bottomMenuBox.addChild(&spacers[0], 0.5f);
    bottomMenuBox.addChild(&homeBtn, 1.0f);
    bottomMenuBox.addChild(&spacers[1], 0.5f);
    bottomMenuBox.addChild(&privacyBtn, 1.0f);

    bottomHBox.addChild(&spacers[2], 1.0f);
    bottomHBox.addChild(&bottomMenuBox, 4.0f);
    bottomHBox.addChild(&spacers[3], 1.0f);

    playBtnBox.addChild(&spacers[4], 2.5f);
    playBtnBox.addChild(&playBtn, 1.0f);
    playBtnBox.addChild(&spacers[5], 2.5f);

    titleHBox.addChild(&spacers[10], 1.0f);
    titleHBox.addChild(&titleLbl, 4.0f); // Adjust this weight to make the title wider or narrower
    titleHBox.addChild(&spacers[11], 1.0f);

    mainVBox.addChild(&spacers[6], 1.5f);
    mainVBox.addChild(&titleHBox, 1.5f);
    mainVBox.addChild(&spacers[7], 1.0f);
    mainVBox.addChild(&playBtnBox, 1.5f);
    mainVBox.addChild(&spacers[8], 3.5f);
    mainVBox.addChild(&bottomHBox, 1.0f);
    mainVBox.addChild(&spacers[9], 0.5f);

    rootStack.addChild(&mainVBox);
}

void MainMenuScene::enter(AppState *state)
{
}

void MainMenuScene::handleEvent(AppState *state, SDL_Event *event)
{
    bool dummy = false;
    rootStack.handleEvent(event, dummy);
}

void MainMenuScene::update(AppState *state)
{
    rootStack.updateLayout(0, 0, state->screenW, state->screenH);
}

void MainMenuScene::render(AppState *state)
{
    SDL_SetRenderDrawColor(state->renderer, 0, 0, 0, 255);
    SDL_RenderClear(state->renderer);

    rootStack.render(state->renderer);

    SDL_RenderPresent(state->renderer);
}