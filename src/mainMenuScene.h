/*
 * mainMenuScene.h
 *
 *      Author: alex@glassoniongames.com
 */

#ifndef MAINMENUSCENE_H
#define MAINMENUSCENE_H

#include "scene.h"
#include "widgets.h"
#include "boxContainer.h"

// Forward declaration
struct AppState;

class MainMenuScene : public Scene
{
public:
    MainMenuScene(AppState *state);
    ~MainMenuScene() override = default;

    void enter(AppState *state) override;
    void handleEvent(AppState *state, SDL_Event *event) override;
    void update(AppState *state) override;
    void render(AppState *state) override;

private:
    Label titleLbl;
    TextureButton onePlayerBtn;
    TextureButton twoPlayerBtn;
    TextureButton resumeBtn;
    TextureButton soundBtn;
    TextureButton homeBtn;
    TextureButton homePageBtn;
    TextureButton privacyBtn;

    StackContainer rootStack;
    VBoxContainer mainVBox;
    HBoxContainer titleHBox;
    HBoxContainer btnWrapperHBox;
    VBoxContainer btnVBox;
    Spacer spacers[15];
};

#endif