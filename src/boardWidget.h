/*
 * boardWidget.h
 *
 *      Author: alex@glassoniongames.com
 */

#ifndef BOARDWIDGET_H
#define BOARDWIDGET_H

#include "widgets.h"
#include "board.h"
#include "gameController.h"
#include <vector>

struct AppState;

class BoardWidget : public Widget
{
public:
    BoardWidget(board &b, GameController &controller, std::vector<MoveRecord> &history, int &historyIndex);
    virtual ~BoardWidget() override = default;

    virtual void updateLayout(float x, float y, float w, float h) override;
    virtual bool handleEvent(const SDL_Event *event, bool &isOver) override;
    virtual void render(SDL_Renderer *renderer) override;

    void setAppState(AppState *state) { this->state = state; }
    float getTileSize() const { return tileSize; }

private:
    board &b;
    GameController &controller;
    std::vector<MoveRecord> &history;
    int &historyIndex;
    AppState *state = nullptr;

    float tileSize = 0;

    void drawCheckerboard(SDL_Renderer *renderer);
    void drawPieceAtPixel(SDL_Renderer *renderer, float centerX, float centerY, char piece);
    void drawPiece(SDL_Renderer *renderer, int row, int col, char piece);
    void drawPieces(SDL_Renderer *renderer);
    void drawSelectedSquare(SDL_Renderer *renderer);
    void drawLegalMoves(SDL_Renderer *renderer);
    void drawMoveAnimation(SDL_Renderer *renderer);
};

#endif