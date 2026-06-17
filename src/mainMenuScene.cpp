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
    onePlayerBtn.setTextures(state->assets.onePlayerTex, state->assets.onePlayerTex, state->assets.onePlayerFilledTex);
    twoPlayerBtn.setTextures(state->assets.twoPlayerTex, state->assets.twoPlayerTex, state->assets.twoPlayerFilledTex);
    resumeBtn.setTextures(state->assets.resumeTex, state->assets.resumeTex, state->assets.resumeFilledTex);
    if (state->soundEnabled)
        soundBtn.setTextures(state->assets.soundOnTex, state->assets.soundOnTex, state->assets.soundOnFilledTex);
    else
        soundBtn.setTextures(state->assets.soundOffTex, state->assets.soundOffTex, state->assets.soundOffFilledTex);

    homePageBtn.setTextures(state->assets.homePageTex, state->assets.homePageTex, state->assets.homePageFilledTex);
    privacyBtn.setTextures(state->assets.privacyTex, state->assets.privacyTex, state->assets.privacyFilledTex);

    // Visually disable resume button if no game is in progress
    if (!state->savedGameScene)
    {
        resumeBtn.alpha = 0.45f;
        resumeBtn.enabled = false;
    }

    // Hook up button events via callbacks
    onePlayerBtn.setOnClickCallback([state]()
                                    {
        state->pvpMode = false;
        state->nextScene = SceneID::NewGame; });

    twoPlayerBtn.setOnClickCallback([state]()
                                    {
        state->pvpMode = true;
        state->nextScene = SceneID::NewGame; });

    resumeBtn.setOnClickCallback([state]()
                                 {
        if (state->savedGameScene) {
            state->startNewGame = false;
            state->nextScene = SceneID::Game;
        } });

    soundBtn.setOnClickCallback([this, state]()
                                {
        state->soundEnabled = !state->soundEnabled;
        if (state->soundEnabled)
            soundBtn.setTextures(state->assets.soundOnTex, state->assets.soundOnTex, state->assets.soundOnFilledTex);
        else
            soundBtn.setTextures(state->assets.soundOffTex, state->assets.soundOffTex, state->assets.soundOffFilledTex); });

    homePageBtn.setOnClickCallback([]()
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
    titleHBox.addChild(&spacers[0], 1.0f);
    titleHBox.addChild(&titleLbl, 4.0f); // Adjust this weight to make the title wider or narrower
    titleHBox.addChild(&spacers[1], 1.0f);

    btnVBox.addChild(&onePlayerBtn, 1.0f);
    btnVBox.addChild(&spacers[2], 0.2f);
    btnVBox.addChild(&twoPlayerBtn, 1.0f);
    btnVBox.addChild(&spacers[3], 0.2f);
    btnVBox.addChild(&resumeBtn, 1.0f);
    btnVBox.addChild(&spacers[4], 0.2f);
    btnVBox.addChild(&soundBtn, 1.0f);
    btnVBox.addChild(&spacers[5], 0.2f);
    btnVBox.addChild(&homePageBtn, 1.0f);
    btnVBox.addChild(&spacers[6], 0.2f);
    btnVBox.addChild(&privacyBtn, 1.0f);

    btnWrapperHBox.addChild(&spacers[7], 1.0f);
    btnWrapperHBox.addChild(&btnVBox, 2.0f);
    btnWrapperHBox.addChild(&spacers[8], 1.0f);

    mainVBox.addChild(&spacers[6], 1.5f);
    mainVBox.addChild(&titleHBox, 1.5f);
    mainVBox.addChild(&spacers[10], 0.5f);
    mainVBox.addChild(&btnWrapperHBox, 6.0f);
    mainVBox.addChild(&spacers[11], 1.0f);

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