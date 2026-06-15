#ifndef GAMESCENE_H
#define GAMESCENE_H

#include "scene.h"
#include "board.h"
#include "gameController.h"
#include "widgets.h"
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
    float tileSize = 0;
    float boardXOffset = 0;
    float boardYOffset = 0;

    board b;
    GameController controller;
    std::vector<MoveRecord> history;
    int historyIndex = 0;
    int winner = 0;

    TextureButton newGameBtn;
    TextureButton undoBtn;
    TextureButton redoBtn;
    TextureButton homeBtn;

    ToggleSwitch *pvpToggle = nullptr;
    Label pvpLabelHuman;
    Label pvpLabelAi;

    DialogBox timeModal;
    Label modalTitle;
    RadioButtonGroup difficultyGroup;
    RadioButton modalOptions[6];
    TextureButton modalStartBtn;

    WorkingIndicator workingIndicator;
    Label redWinLbl;
    Label blackWinLbl;
    Label searchDepthLbl;
    int lastDisplayedDepth = -1;

    void updateLayout(AppState *state);
    void replayHistory(AppState *state);

    void drawCheckerboard(SDL_Renderer *renderer, AppState *state);
    void drawPieceAtPixel(SDL_Renderer *renderer, float centerX, float centerY, char piece, AppState *state);
    void drawPiece(SDL_Renderer *renderer, int row, int col, char piece, AppState *state);
    void drawPieces(SDL_Renderer *renderer, AppState *state);
    void drawSelectedSquare(SDL_Renderer *renderer, AppState *state);
    void drawLegalMoves(SDL_Renderer *renderer, AppState *state);
    void drawMoveAnimation(SDL_Renderer *renderer, AppState *state);
    void drawGameOverMessage(SDL_Renderer *renderer, AppState *state);
};

#endif