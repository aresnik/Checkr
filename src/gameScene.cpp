#include "gameScene.h"
#include "appState.h"
#include <iostream>
#include <string>

GameScene::GameScene(AppState *state)
{
    newGameBtn.setTextures(state->assets.newGameTex, state->assets.newGameTex, state->assets.newGameFilledTex);
    undoBtn.setTextures(state->assets.undoTex, state->assets.undoTex, state->assets.undoFilledTex);
    redoBtn.setTextures(state->assets.redoTex, state->assets.redoTex, state->assets.redoFilledTex);

    homeBtn.setTextures(state->assets.homeTex, state->assets.homeTex, state->assets.homeFilledTex);

    if (state->assets.font && state->assets.uiFont)
    {
        timeModal.setColors({20, 20, 20, 240}, {255, 255, 255, 255});
        timeModal.setBorderWidth(2.0f);

        pvpLabelHuman.load(state->renderer, state->assets.uiFont, "Human", {255, 255, 255, 255});
        pvpLabelAi.load(state->renderer, state->assets.uiFont, "AI", {255, 255, 255, 255});

        pvpToggle = new ToggleSwitch({0, 0, 55, 20});
        if (!controller.pvpMode)
            pvpToggle->setToggled(true);

        modalTitle.load(state->renderer, state->assets.uiFont, "Select AI Difficulty", {255, 255, 255, 255});

        int times[] = {3, 5, 10, 20, 30, 60};
        for (int i = 0; i < 6; ++i)
        {
            modalOptions[i].setValue(times[i]);
            std::string txt = std::to_string(times[i]) + " seconds";
            if (times[i] == 60)
                txt = "1 minute";
            modalOptions[i].getLabel().load(state->renderer, state->assets.uiFont, txt, {220, 220, 220, 255});
            difficultyGroup.addButton(&modalOptions[i]);
        }
        difficultyGroup.setSelectedValue(3);
        timeModal.addChild(&difficultyGroup);

        modalStartBtn.setTextures(state->assets.okTex, state->assets.okTex, state->assets.okFilledTex);
        modalStartBtn.setOnClickCallback([this, state]()
                                         {
            controller.pvpMode = (pvpToggle ? !pvpToggle->isSwitchToggled() : false);
            controller.aiTimeLimit = difficultyGroup.getSelectedValue();
            SDL_Color white = {255, 255, 255, 255};
            blackWinLbl.load(state->renderer, state->assets.font, "BLACK WINS!", white);
            b.startup();
            timeModal.visible = false;
            winner = 0;
            history.clear();
            historyIndex = 0; });
        timeModal.addChild(&modalStartBtn);
        timeModal.visible = true;

        SDL_Color white = {255, 255, 255, 255};
        SDL_Color red = {200, 40, 40, 255};
        redWinLbl.load(state->renderer, state->assets.font, "RED WINS!", red);
        blackWinLbl.load(state->renderer, state->assets.font, "BLACK WINS!", white);
    }
}

GameScene::~GameScene()
{
    SDL_Log("Destroying GameScene, shutting down AI thread safely...");
    controller.stopAI();
    if (pvpToggle)
        delete pvpToggle;
}

void GameScene::enter(AppState *state) {}

void GameScene::updateLayout(AppState *state)
{
    tileSize = 400.0f / 8.0f;
    boardXOffset = 0.0f;
    boardYOffset = 200.0f;

    float btnSize = tileSize * 1.0f;
    float spacing = tileSize * 0.5f;
    float totalWidth = tileSize * 8.0f;

    workingIndicator.active = controller.aiThinking;
    workingIndicator.updateLayout(boardXOffset + (totalWidth - 100) / 2, boardYOffset - 40, 100, 30);

    timeModal.updateLayout(40, 200, 320, 400);

    float mRectX = timeModal.rect.x;
    float mRectY = timeModal.rect.y;
    pvpLabelHuman.updateLayout(mRectX + 40, mRectY + 25, 80, 30);
    pvpLabelAi.updateLayout(mRectX + 215, mRectY + 25, 30, 30);
    if (pvpToggle)
    {
        pvpToggle->updateLayout(mRectX + 135, mRectY + 30, 55, 20);
        pvpToggle->alpha = 1.0f;
    }
    modalTitle.updateLayout(mRectX + 20, mRectY + 70, 280, 40);
    difficultyGroup.updateLayout(mRectX + 30, mRectY + 130, 25, 25);
    modalStartBtn.updateLayout(mRectX + 320 - btnSize - 20, mRectY + 400 - btnSize - 20, btnSize, btnSize);

    bool aiSelected = pvpToggle ? pvpToggle->isSwitchToggled() : true;
    modalTitle.alpha = aiSelected ? 1.0f : 0.45f;
    difficultyGroup.alpha = 1.0f;
    difficultyGroup.enabled = aiSelected;

    float msgW = tileSize * 4.0f;
    float msgH = tileSize * 1.0f;
    float msgX = boardXOffset + (tileSize * 8.0f - msgW) / 2.0f;
    float msgY = newGameBtn.rect.y + newGameBtn.rect.h + (tileSize * 0.2f);
    redWinLbl.updateLayout(msgX, msgY, msgW, msgH);
    blackWinLbl.updateLayout(msgX, msgY, msgW, msgH);

    int currentDepth = controller.currentSearchDepth.load();
    if (currentDepth != lastDisplayedDepth)
    {
        searchDepthLbl.load(state->renderer, state->assets.uiFont, "Depth: " + std::to_string(currentDepth), {200, 200, 200, 255});
        lastDisplayedDepth = currentDepth;
    }
    searchDepthLbl.updateLayout(boardXOffset + 10, boardYOffset - 35, 80, 25);

    float bottomBtnSize = tileSize * 1.0f;
    float largeGap = tileSize * 0.9f;
    float bottomGroupWidth = (2.0f * bottomBtnSize) + largeGap;
    float bottomStartX = (400.0f - bottomGroupWidth) / 2.0f;
    float btnYBottom = boardYOffset + (tileSize * 8.0f) + (tileSize * 0.5f);

    newGameBtn.updateLayout(bottomStartX, btnYBottom, bottomBtnSize, bottomBtnSize);
    homeBtn.updateLayout(bottomStartX + bottomBtnSize + largeGap, btnYBottom, bottomBtnSize, bottomBtnSize);

    float btnYTop = boardYOffset - btnSize - (tileSize * 1.2f);
    float pairWidth = (btnSize * 2.0f) + spacing;
    float startX = boardXOffset + (totalWidth - pairWidth) / 2.0f;

    undoBtn.updateLayout(startX, btnYTop, btnSize, btnSize);
    redoBtn.updateLayout(startX + btnSize + spacing, btnYTop, btnSize, btnSize);

    bool engineIdle = !controller.aiThinking && !controller.animation.active;
    undoBtn.enabled = engineIdle && (historyIndex > 0);
    redoBtn.enabled = engineIdle && (historyIndex < (int)history.size());
    newGameBtn.enabled = engineIdle;
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
    if (timeModal.visible)
    {
        bool isOverModal = false;
        timeModal.handleEvent(event, isOverModal);
        if (pvpToggle)
            pvpToggle->handleEvent(*event);
        return;
    }

    bool overNew = false, overUndo = false, overRedo = false, overHome = false;

    bool clickedNew = newGameBtn.handleEvent(event, overNew);
    bool clickedUndo = undoBtn.handleEvent(event, overUndo);
    bool clickedRedo = redoBtn.handleEvent(event, overRedo);
    bool clickedHome = homeBtn.handleEvent(event, overHome);

    if (clickedNew)
        timeModal.visible = true;
    else if (clickedHome)
    {
        state->nextScene = SceneID::MainMenu;
    }
    else if (clickedUndo)
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
        }
    }
    else if (clickedRedo)
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
        }
    }
    else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
        if (event->button.button == SDL_BUTTON_LEFT && !overNew && !overUndo && !overRedo && !overHome)
        {
            float adjustedX = event->button.x - boardXOffset;
            float adjustedY = event->button.y - boardYOffset;
            int col = static_cast<int>(adjustedX / tileSize);
            int row = static_cast<int>(adjustedY / tileSize);
            if (adjustedX >= 0 && adjustedY >= 0 && col < 8 && row < 8)
            {
                controller.handleClick(b, row, col, history, historyIndex);
            }
        }
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

// --- Rendering Helpers ---
void GameScene::drawCheckerboard(SDL_Renderer *renderer, AppState *state)
{
    float boardDim = tileSize * 8.0f;
    SDL_FRect dst = {boardXOffset, boardYOffset, boardDim, boardDim};
    SDL_RenderTexture(renderer, state->assets.boardTexture, NULL, &dst);
}
void GameScene::drawPieceAtPixel(SDL_Renderer *renderer, float centerX, float centerY, char piece, AppState *state)
{
    float radius = tileSize * 0.40f;
    SDL_Texture *tex = nullptr;
    if (piece == 'b')
        tex = state->assets.blackTexture;
    else if (piece == 'r')
        tex = state->assets.redTexture;
    else if (piece == 'R')
        tex = state->assets.redKingTexture;
    else if (piece == 'B')
        tex = state->assets.blackKingTexture;
    if (tex)
    {
        SDL_FRect dst = {centerX - radius, centerY - radius, radius * 2.0f, radius * 2.0f};
        SDL_RenderTexture(renderer, tex, NULL, &dst);
    }
}
void GameScene::drawPiece(SDL_Renderer *renderer, int row, int col, char piece, AppState *state)
{
    float centerX = col * tileSize + tileSize / 2.0f + boardXOffset;
    float centerY = row * tileSize + tileSize / 2.0f + boardYOffset;
    drawPieceAtPixel(renderer, centerX, centerY, piece, state);
}
void GameScene::drawPieces(SDL_Renderer *renderer, AppState *state)
{
    for (int row = 0; row < 8; ++row)
    {
        for (int col = 0; col < 8; ++col)
        {
            char piece = b.getPieceAt8x8(row, col);
            if (piece == 'e')
                continue;
            if (controller.animation.active)
            {
                bool isPending = false;
                for (const auto &cp : controller.pendingCaptures)
                {
                    if (cp.row == row && cp.col == col)
                    {
                        isPending = true;
                        break;
                    }
                }
                if (isPending)
                    continue;
            }
            if (controller.animation.active)
            {
                int hideRow = controller.animation.toRow;
                int hideCol = controller.animation.toCol;
                if (!controller.animationPath.empty())
                {
                    hideRow = controller.animationPath.back().row;
                    hideCol = controller.animationPath.back().col;
                }
                if (row == hideRow && col == hideCol)
                    continue;
            }
            drawPiece(renderer, row, col, piece, state);
        }
    }
    for (const auto &cap : controller.pendingCaptures)
    {
        bool shouldDraw = false;
        Uint64 elapsed = SDL_GetTicks() - controller.animation.startTime;
        float t = static_cast<float>(elapsed) / static_cast<float>(controller.animation.durationMs);
        if (controller.animation.isUndo)
        {
            if (controller.animationPathIndex > cap.captureStep)
                shouldDraw = true;
            else if (controller.animationPathIndex == cap.captureStep && t >= 0.5f)
                shouldDraw = true;
        }
        else
        {
            if (controller.animationPathIndex < cap.captureStep)
                shouldDraw = true;
            else if (controller.animationPathIndex == cap.captureStep && t < 0.5f)
                shouldDraw = true;
        }
        if (shouldDraw)
            drawPiece(renderer, cap.row, cap.col, cap.piece, state);
    }
}
void GameScene::drawSelectedSquare(SDL_Renderer *renderer, AppState *state)
{
    if (controller.selectedRow < 0 || controller.selectedCol < 0)
        return;
    SDL_FRect highlight;
    highlight.x = controller.selectedCol * tileSize + boardXOffset;
    highlight.y = controller.selectedRow * tileSize + boardYOffset;
    highlight.w = tileSize;
    highlight.h = tileSize;
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderRect(renderer, &highlight);
}
void GameScene::drawLegalMoves(SDL_Renderer *renderer, AppState *state)
{
    float radius = tileSize * 0.15f;
    for (const auto &move : controller.legalMoves)
    {
        float centerX = move.col * tileSize + tileSize / 2.0f + boardXOffset;
        float centerY = move.row * tileSize + tileSize / 2.0f + boardYOffset;
        SDL_FRect dst = {centerX - radius, centerY - radius, radius * 2.0f, radius * 2.0f};
        SDL_RenderTexture(renderer, state->assets.legalMoveTexture, NULL, &dst);
    }
}
void GameScene::drawMoveAnimation(SDL_Renderer *renderer, AppState *state)
{
    MoveAnimation &animation = controller.animation;
    if (!animation.active)
        return;
    Uint64 now = SDL_GetTicks();
    Uint64 elapsed = now - animation.startTime;
    if (elapsed >= (Uint64)animation.durationMs)
    {
        animation.active = false;
        return;
    }
    float t = static_cast<float>(elapsed) / static_cast<float>(animation.durationMs);
    float fromX = animation.fromCol * tileSize + tileSize / 2.0f + boardXOffset;
    float fromY = animation.fromRow * tileSize + tileSize / 2.0f + boardYOffset;
    float toX = animation.toCol * tileSize + tileSize / 2.0f + boardXOffset;
    float toY = animation.toRow * tileSize + tileSize / 2.0f + boardYOffset;
    float currentX = fromX + (toX - fromX) * t;
    float currentY = fromY + (toY - fromY) * t;
    drawPieceAtPixel(renderer, currentX, currentY, animation.piece, state);
}
void GameScene::drawGameOverMessage(SDL_Renderer *renderer, AppState *state)
{
    if (winner == 1)
        redWinLbl.render(renderer);
    else if (winner == 2)
        blackWinLbl.render(renderer);
}

void GameScene::render(AppState *state)
{
    SDL_SetRenderDrawColor(state->renderer, 0, 0, 0, 255);
    SDL_RenderClear(state->renderer);

    drawCheckerboard(state->renderer, state);
    drawSelectedSquare(state->renderer, state);
    drawLegalMoves(state->renderer, state);
    drawPieces(state->renderer, state);
    drawMoveAnimation(state->renderer, state);

    newGameBtn.render(state->renderer);
    undoBtn.render(state->renderer);
    redoBtn.render(state->renderer);
    homeBtn.render(state->renderer);

    drawGameOverMessage(state->renderer, state);
    searchDepthLbl.render(state->renderer);
    workingIndicator.render(state->renderer);
    timeModal.render(state->renderer);

    if (timeModal.visible)
    {
        modalTitle.render(state->renderer);
        pvpLabelHuman.render(state->renderer);
        pvpLabelAi.render(state->renderer);
        if (pvpToggle)
            pvpToggle->render(state->renderer);
    }

    SDL_RenderPresent(state->renderer);
}