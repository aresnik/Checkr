#ifndef MAINMENUSCENE_H
#define MAINMENUSCENE_H

#include "scene.h"
#include "widgets.h"

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
    TextureButton playBtn;
    TextureButton soundBtn;
    TextureButton homeBtn;
    TextureButton privacyBtn;
};

#endif