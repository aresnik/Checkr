/*
 * gameScene.cpp
 *
 *      Author: alex@glassoniongames.com
 */

#include "gameScene.h"
#include "appState.h"
#include <iostream>
#include <string>
#include <algorithm>

GameScene::GameScene(AppState *state)
{
    boardWidget.setAppState(state);

    newGameBtn.setTextures(state->assets.newGameTex, state->assets.newGameTex, state->assets.newGameFilledTex);
    undoBtn.setTextures(state->assets.undoTex, state->assets.undoTex, state->assets.undoFilledTex);
    redoBtn.setTextures(state->assets.redoTex, state->assets.redoTex, state->assets.redoFilledTex);

    homeBtn.setTextures(state->assets.homeTex, state->assets.homeTex, state->assets.homeFilledTex);

    newGameBtn.setOnClickCallback([this]()
                                  { modalPadVBox.visible = true; });
    homeBtn.setOnClickCallback([state]()
                               { state->nextScene = SceneID::MainMenu; });
    undoBtn.setOnClickCallback([this, state]()
                               {
        if (historyIndex > 0)
        {
            MoveRecord m = history[historyIndex - 1];
            historyIndex--;
            replayHistory(state);
            char piece = b.getPieceAt8x8(m.fromRow, m.fromCol);
            controller.setupPathAnimation(b, piece, m.fromRow, m.fromCol, m.toRow, m.toCol, true);
            controller.selectedRow = -1;
            controller.selectedCol = -1;
            controller.legalMoves.clear();
            winner = 0;
        } });
    redoBtn.setOnClickCallback([this]()
                               {
        if (historyIndex < (int)history.size())
        {
            MoveRecord m = history[historyIndex];
            char piece = b.getPieceAt8x8(m.fromRow, m.fromCol);
            controller.setupPathAnimation(b, piece, m.fromRow, m.fromCol, m.toRow, m.toCol);
            b.tryMove8x8(m.fromRow, m.fromCol, m.toRow, m.toCol);
            historyIndex++;
            controller.selectedRow = -1;
            controller.selectedCol = -1;
            controller.legalMoves.clear();
            winner = 0;
        } });

    topBtnBox.addChild(&spacers[0], 2.5f);
    topBtnBox.addChild(&undoBtn, 1.0f);
    topBtnBox.addChild(&redoBtn, 1.0f);
    topBtnBox.addChild(&spacers[1], 2.5f);

    bottomBtnBox.addChild(&spacers[2], 0.25f);
    bottomBtnBox.addChild(&newGameBtn, 1.0f);
    bottomBtnBox.addChild(&spacers[3], 2.5f);
    bottomBtnBox.addChild(&homeBtn, 1.0f);
    bottomBtnBox.addChild(&spacers[4], 0.25f);

    msgHBox.addChild(&spacers[6], 2.0f);
    msgHBox.addChild(&msgStack, 1.0f);
    msgHBox.addChild(&spacers[7], 2.0f);
    redWinLbl.setAlignment(Label::ALIGN_CENTER);
    blackWinLbl.setAlignment(Label::ALIGN_CENTER);

    if (state->assets.font && state->assets.uiFont)
    {
        setupDialog.load(state, controller.pvpMode);
        setupDialog.setOnStartCallback([this, state](bool pvpMode, int timeLimit)
                                       {
            controller.pvpMode = pvpMode;
            controller.aiTimeLimit = timeLimit;
            SDL_Color white = {255, 255, 255, 255};
            blackWinLbl.load(state->renderer, state->assets.font, "BLACK WINS!", white);
            b.startup();
            modalPadVBox.visible = false;
            winner = 0;
            history.clear();
            historyIndex = 0; });

        SDL_Color white = {255, 255, 255, 255};
        SDL_Color red = {200, 40, 40, 255};
        redWinLbl.load(state->renderer, state->assets.font, "RED WINS!", red);
        blackWinLbl.load(state->renderer, state->assets.font, "BLACK WINS!", white);
    }

    searchDepthLbl.setAlignment(Label::ALIGN_LEFT);
    indicatorHBox.addChild(&spacers[9], 0.02f);
    indicatorHBox.addChild(&searchDepthLbl, 1.2f);
    indicatorHBox.addChild(&spacers[23], 0.02f);
    indicatorHBox.addChild(&workingIndicator, 0.4f);
    indicatorHBox.addChild(&spacers[10], 1.2f);

    // Group the buttons, indicator, board, and messages together perfectly using Aspect Ratio!
    // Top buttons (10%), Indicator (5%), Board (100%), MsgStack (10%), Bottom buttons (10%). Total height = 1.35.
    boardAspect.setRatio(1.0f / 1.39f);
    boardGroupVBox.addChild(&topBtnBox, 0.12f);
    boardGroupVBox.addChild(&indicatorHBox, 0.05f);
    boardGroupVBox.addChild(&boardWidget, 1.0f);
    boardGroupVBox.addChild(&msgStack, 0.05f);
    boardGroupVBox.addChild(&bottomBtnBox, 0.12f);
    boardAspect.addChild(&boardGroupVBox);

    mainVBox.addChild(&spacers[5], 0.02f);
    mainVBox.addChild(&boardAspect, 1.0f); // Board aspect container will manage max dimensions safely
    mainVBox.addChild(&spacers[24], 0.02f);

    msgStack.addChild(&redWinLbl);
    msgStack.addChild(&blackWinLbl);

    modalAspect.setRatio(1.0f / 1.4f);
    modalAspect.addChild(&setupDialog);

    modalPadHBox.addChild(&spacers[15], 0.5f);
    modalPadHBox.addChild(&modalAspect, 5.0f);
    modalPadHBox.addChild(&spacers[16], 0.5f);

    modalPadVBox.addChild(&spacers[17], 0.7f);
    modalPadVBox.addChild(&modalPadHBox, 4.0f);
    modalPadVBox.addChild(&spacers[18], 0.9f);

    rootStack.addChild(&mainVBox);
    rootStack.addChild(&modalPadVBox);

    modalPadVBox.visible = true;
}

GameScene::~GameScene()
{
    SDL_Log("Destroying GameScene, shutting down AI thread safely...");
    controller.stopAI();
}

void GameScene::enter(AppState *state) {}

void GameScene::updateLayout(AppState *state)
{
    int currentDepth = controller.currentSearchDepth.load();
    if (currentDepth != lastDisplayedDepth)
    {
        searchDepthLbl.load(state->renderer, state->assets.uiFont, "Depth: " + std::to_string(currentDepth), {200, 200, 200, 255});
        lastDisplayedDepth = currentDepth;
    }

    bool engineIdle = !controller.aiThinking && !controller.animation.active;
    undoBtn.enabled = engineIdle && (historyIndex > 0);
    redoBtn.enabled = engineIdle && (historyIndex < (int)history.size());
    newGameBtn.enabled = engineIdle;

    redWinLbl.visible = (winner == 1);
    blackWinLbl.visible = (winner == 2);
    workingIndicator.active = controller.aiThinking;

    rootStack.updateLayout(0, 0, state->screenW, state->screenH);
}

void GameScene::replayHistory(AppState *state)
{
    b.startup();
    for (int i = 0; i < historyIndex; ++i)
    {
        const auto &m = history[i];
        b.tryMove8x8(m.fromRow, m.fromCol, m.toRow, m.toCol);
    }
}

void GameScene::handleEvent(AppState *state, SDL_Event *event)
{
    bool dummy = false;
    if (modalPadVBox.visible)
    {
        modalPadVBox.handleEvent(event, dummy);
    }
    else
    {
        rootStack.handleEvent(event, dummy);
    }
}

void GameScene::update(AppState *state)
{
    updateLayout(state);
    controller.updateAI(b, history, historyIndex);
    controller.updateAnimation();

    if (controller.soundTrigger > 0)
    {
        if (state->mixer && state->soundEnabled)
        {
            MIX_PlayAudio(state->mixer, controller.soundTrigger == 2 ? state->assets.captureSfx : state->assets.moveSfx);
        }
        controller.soundTrigger = 0;
    }

    if (!controller.animation.active && winner == 0 && b.terminalTest())
    {
        winner = (b.getTurnPublic() == 'r') ? 2 : 1;
        std::cout << "Game Over! Winner: " << (winner == 1 ? "Red" : "Black") << std::endl;
        if (state->mixer && state->assets.winSfx && state->soundEnabled)
            MIX_PlayAudio(state->mixer, state->assets.winSfx);
    }
}

void GameScene::render(AppState *state)
{
    SDL_SetRenderDrawColor(state->renderer, 0, 0, 0, 255);
    SDL_RenderClear(state->renderer);

    rootStack.render(state->renderer);

    SDL_RenderPresent(state->renderer);
}