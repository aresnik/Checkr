/*
 * newGameScene.cpp
 *
 *      Author: alex@glassoniongames.com
 */

#include "newGameScene.h"
#include "appState.h"

NewGameScene::NewGameScene(AppState *state)
{
    if (state->assets.font)
    {
        if (state->pvpMode)
            titleLbl.load(state->renderer, state->assets.font, "Human to Human Player", {255, 255, 255, 255});
        else
            titleLbl.load(state->renderer, state->assets.font, "SELECT DIFFICULTY", {255, 255, 255, 255});

        titleLbl.setAlignment(Label::ALIGN_CENTER);
    }

    difficultySlider.setNumPositions(5);
    difficultySlider.setValue(state->aiDifficulty); // Default to current state
    if (state->assets.uiFont)
    {
        SDL_Color white = {255, 255, 255, 255};
        difficultySlider.setLabel(0, state->renderer, state->assets.uiFont, "3sec", white);
        difficultySlider.setLabel(1, state->renderer, state->assets.uiFont, "5sec", white);
        difficultySlider.setLabel(2, state->renderer, state->assets.uiFont, "15sec", white);
        difficultySlider.setLabel(3, state->renderer, state->assets.uiFont, "30sec", white);
        difficultySlider.setLabel(4, state->renderer, state->assets.uiFont, "1min", white);
    }

    startBtn.setTextures(state->assets.newGameTex, state->assets.newGameTex, state->assets.newGameFilledTex);
    startBtn.setOnClickCallback([this, state]()
                                {
                                    if (!state->pvpMode)
                                        state->aiDifficulty = difficultySlider.getValue();
                                    state->startNewGame = true;
                                    state->nextScene = SceneID::Game; });

    homeBtn.setTextures(state->assets.homeTex, state->assets.homeTex, state->assets.homeFilledTex);
    homeBtn.setOnClickCallback([state]()
                               { state->nextScene = SceneID::MainMenu; });

    btnHBox.addChild(&spacers[1], 2.5f);
    btnHBox.addChild(&startBtn, 1.0f);
    btnHBox.addChild(&spacers[2], 2.5f);

    homeBtnHBox.addChild(&spacers[6], 2.5f);
    homeBtnHBox.addChild(&homeBtn, 1.0f);
    homeBtnHBox.addChild(&spacers[7], 2.5f);

    // Build the basic UI tree with flex containers
    mainVBox.addChild(&spacers[0], 1.0f);
    mainVBox.addChild(&titleLbl, 1.5f);
    mainVBox.addChild(&spacers[3], 2.0f);

    if (!state->pvpMode)
    {
        mainVBox.addChild(&difficultySlider, 1.5f);
        mainVBox.addChild(&spacers[4], 2.0f);
    }
    else
    {
        mainVBox.addChild(&spacers[4], 3.5f); // Keep consistent spacing if slider is removed
    }

    mainVBox.addChild(&btnHBox, 1.5f);
    mainVBox.addChild(&spacers[8], 0.2f);
    mainVBox.addChild(&homeBtnHBox, 1.2f);
    mainVBox.addChild(&spacers[5], 0.8f);

    rootStack.addChild(&mainVBox);
}

void NewGameScene::enter(AppState *state)
{
}

void NewGameScene::handleEvent(AppState *state, SDL_Event *event)
{
    bool dummy = false;
    rootStack.handleEvent(event, dummy);
}

void NewGameScene::update(AppState *state)
{
    rootStack.updateLayout(0, 0, state->screenW, state->screenH);
}

void NewGameScene::render(AppState *state)
{
    SDL_SetRenderDrawColor(state->renderer, 20, 20, 20, 255);
    SDL_RenderClear(state->renderer);

    rootStack.render(state->renderer);

    SDL_RenderPresent(state->renderer);
}