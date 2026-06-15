#ifndef SCENE_H
#define SCENE_H

#include <SDL3/SDL.h>

// Forward declaration of AppState so scenes can access shared resources
// (like the renderer) and trigger scene transitions.
struct AppState;

class Scene
{
public:
    virtual ~Scene() = default;

    // Called when the scene is first loaded to set up animations/layout
    virtual void enter(AppState *state) {}

    // Called when the application receives an SDL event
    virtual void handleEvent(AppState *state, SDL_Event *event) = 0;

    // Called every frame to update logic and render
    virtual void update(AppState *state) = 0;
    virtual void render(AppState *state) = 0;
};

#endif