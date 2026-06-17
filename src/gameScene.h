/*
 * gameScene.h
 *
 *      Author: alex@glassoniongames.com
 */

#ifndef GAMESCENE_H
#define GAMESCENE_H

#include "scene.h"
#include "board.h"
#include "gameController.h"
#include "widgets.h"
#include "boxContainer.h"
#include "boardWidget.h"
#include <vector>

// Forward declaration to avoid circular dependencies
struct AppState;

class GameScene : public Scene
{
public:
    GameScene(AppState *state);
    ~GameScene() override;

    void enter(AppState *state) override;
    void handleEvent(AppState *state, SDL_Event *event) override;
    void update(AppState *state) override;
    void render(AppState *state) override;

private:
    board b;
    GameController controller;
    std::vector<MoveRecord> history;
    int historyIndex = 0;
    int winner = 0;

    BoardWidget boardWidget{b, controller, history, historyIndex};

    TextureButton newGameBtn;
    TextureButton undoBtn;
    TextureButton redoBtn;
    TextureButton homeBtn;

    StackContainer rootStack;
    VBoxContainer mainVBox;

    StackContainer msgStack;
    HBoxContainer msgHBox;

    HBoxContainer indicatorHBox;

    VBoxContainer boardGroupVBox;
    AspectRatioContainer boardAspect;

    HBoxContainer topBtnBox;
    HBoxContainer bottomBtnBox;

    WorkingIndicator workingIndicator;
    Label redWinLbl;
    Label blackWinLbl;
    Label searchDepthLbl;
    int lastDisplayedDepth = -1;

    Spacer spacers[26];

    void updateLayout(AppState *state);
    void replayHistory(AppState *state);
};

#endif