/*
 * gameSetupDialog.h
 *
 *      Author: alex@glassoniongames.com
 */

#ifndef GAMESETUPDIALOG_H
#define GAMESETUPDIALOG_H

#include "widgets.h"
#include "appState.h"
#include <functional>

class GameSetupDialog : public Widget
{
public:
    GameSetupDialog() = default;
    ~GameSetupDialog() override;

    void load(AppState *state, bool initialPvpMode);

    void render(SDL_Renderer *renderer) override;
    bool handleEvent(const SDL_Event *event, bool &isOver) override;
    void updateLayout(float x, float y, float w, float h) override;

    void setOnStartCallback(std::function<void(bool pvp, int timeLimit)> cb) { onStart = cb; }

private:
    DialogBox background;
    Label modalTitle;
    Label pvpLabelHuman;
    Label pvpLabelAi;
    ToggleSwitch *pvpToggle = nullptr;
    RadioButtonGroup difficultyGroup;
    RadioButton modalOptions[6];
    TextureButton modalStartBtn;

    std::function<void(bool, int)> onStart;
};

#endif // GAMESETUPDIALOG_H