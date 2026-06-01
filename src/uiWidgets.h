#ifndef UIWIDGETS_H
#define UIWIDGETS_H

#include <SDL3/SDL.h>
#include <string>

class UITextureButton {
public:
    SDL_FRect rect = { 0, 0, 0, 0 };
    SDL_Texture* texture = nullptr;
    SDL_Texture* pressedTexture = nullptr;
    bool isPressed = false;
    bool enabled = true;

    UITextureButton() = default;
    ~UITextureButton();

    // Loads textures directly from file paths. 
    bool load(SDL_Renderer* renderer, const std::string& path, const std::string& pressedPath = "");

    // Safely destroys owned textures
    void cleanup();

    // Updates the position and size of the button
    void updateLayout(float x, float y, float w, float h);

    // Processes mouse events. 
    // Returns true only if the mouse was pressed AND released over the button.
    // 'isOver' is an output parameter used to block clicks from hitting the board underneath.
    bool handleEvent(const SDL_Event* event, bool& isOver);

    // Renders the button. 
    // Automatically handles pressed states and dims the button if 'enabled' is false.
    void render(SDL_Renderer* renderer);
};

#endif
