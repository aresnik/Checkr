/*
 * gameSetupDialog.cpp
 *
 *      Author: alex@glassoniongames.com
 */

#include "gameSetupDialog.h"
#include <string>

GameSetupDialog::~GameSetupDialog()
{
    if (pvpToggle)
        delete pvpToggle;
}

void GameSetupDialog::load(AppState *state, bool initialPvpMode)
{
    background.setColors({20, 20, 20, 240}, {255, 255, 255, 255});
    background.setBorderWidth(2.0f);

    if (state->assets.uiFont)
    {
        modalTitle.load(state->renderer, state->assets.uiFont, "Select AI Difficulty", {255, 255, 255, 255});
        modalTitle.setAlignment(Label::ALIGN_CENTER);

        pvpLabelHuman.load(state->renderer, state->assets.uiFont, "Human", {255, 255, 255, 255});
        pvpLabelHuman.setAlignment(Label::ALIGN_RIGHT);

        pvpLabelAi.load(state->renderer, state->assets.uiFont, "AI", {255, 255, 255, 255});
        pvpLabelAi.setAlignment(Label::ALIGN_LEFT);

        if (!pvpToggle)
            pvpToggle = new ToggleSwitch({0, 0, 55, 20});
        
        if (!initialPvpMode)
            pvpToggle->setToggled(true);

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

        modalStartBtn.setTextures(state->assets.okTex, state->assets.okTex, state->assets.okFilledTex);
        modalStartBtn.setOnClickCallback([this]()
        {
            if (onStart)
            {
                bool pvpMode = (pvpToggle ? !pvpToggle->isSwitchToggled() : false);
                int timeLimit = difficultyGroup.getSelectedValue();
                onStart(pvpMode, timeLimit);
            }
        });
    }
}

void GameSetupDialog::updateLayout(float x, float y, float w, float h)
{
    rect = {x, y, w, h};
    
    background.updateLayout(x, y, w, h);

    // Hardcode placement using percentages of width and height
    modalTitle.updateLayout(x, y + h * 0.05f, w, h * 0.07f);

    float pvpY = y + h * 0.15f;
    float pvpH = h * 0.15f;
    float toggleW = w * 0.20f;
    float toggleX = x + (w - toggleW) / 2.0f;

    float lblH = pvpH * 0.4f; 
    float lblY = pvpY + (pvpH - lblH) / 2.0f;
    
    pvpLabelHuman.updateLayout(x, lblY, toggleX - 15.0f - x, lblH);
    if (pvpToggle)
        pvpToggle->updateLayout(toggleX, pvpY, toggleW, pvpH);
    pvpLabelAi.updateLayout(toggleX + toggleW + 15.0f, lblY, x + w - (toggleX + toggleW + 15.0f), lblH);

    difficultyGroup.updateLayout(x + w * 0.15f, y + h * 0.33f, w * 0.7f, h * 0.40f);

    float btnW = w * 0.4f;
    float btnH = h * 0.12f;
    modalStartBtn.updateLayout(x + (w - btnW) / 2.0f, y + h * 0.82f, btnW, btnH);
}

void GameSetupDialog::render(SDL_Renderer *renderer)
{
    if (!visible) return;
    
    // Dynamically update visual state
    bool aiSelected = pvpToggle ? pvpToggle->isSwitchToggled() : true;
    modalTitle.alpha = aiSelected ? 1.0f : 0.45f;
    difficultyGroup.alpha = 1.0f;
    difficultyGroup.enabled = aiSelected;

    background.render(renderer);
    modalTitle.render(renderer);
    pvpLabelHuman.render(renderer);
    if (pvpToggle)
        pvpToggle->render(renderer);
    pvpLabelAi.render(renderer);
    difficultyGroup.render(renderer);
    modalStartBtn.render(renderer);
}

bool GameSetupDialog::handleEvent(const SDL_Event *event, bool &isOver)
{
    if (!visible || !enabled)
    {
        isOver = false;
        return false;
    }

    bool handled = false;
    bool childOver = false;

    if (modalStartBtn.handleEvent(event, childOver)) handled = true;
    if (difficultyGroup.handleEvent(event, childOver)) handled = true;
    if (pvpToggle && pvpToggle->handleEvent(event, childOver)) handled = true;
    
    isOver = childOver;

    // Consume mouse events inside the dialog boundary
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
        float mx = event->button.x;
        float my = event->button.y;
        if (mx >= rect.x && mx <= rect.x + rect.w && my >= rect.y && my <= rect.y + rect.h)
        {
            isOver = true;
            handled = true;
        }
    }
    return handled;
}