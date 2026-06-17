/*
 * newGameScene.h
 *
 *      Author: alex@glassoniongames.com
 */

#ifndef NEWGAMESCENE_H
#define NEWGAMESCENE_H

#include <SDL3/SDL.h>
#include "scene.h"
#include "appState.h"
#include "widgets.h"

class NewGameScene : public Scene
{
public:
    NewGameScene(AppState *state);
    virtual ~NewGameScene() = default;

    void enter(AppState *state) override;
    void handleEvent(AppState *state, SDL_Event *event) override;
    void update(AppState *state) override;
    void render(AppState *state) override;

private:
    StackContainer rootStack;
    VBoxContainer mainVBox;
    HBoxContainer btnHBox;
    HBoxContainer homeBtnHBox;

    Label titleLbl;
    Slider difficultySlider;
    TextureButton startBtn;
    TextureButton homeBtn;

    // Reusable empty spacers for flex layouts
    Spacer spacers[10];
};

#endif