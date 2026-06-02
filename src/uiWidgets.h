#ifndef UIWIDGETS_H
#define UIWIDGETS_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>
#include <vector>

class UIWidget
{
public:
    SDL_FRect rect = {0, 0, 0, 0};
    bool enabled = true;
    virtual ~UIWidget() = default;
    virtual void updateLayout(float x, float y, float w, float h) { rect = {x, y, w, h}; }
    virtual void render(SDL_Renderer *renderer) = 0;
    // Returns true if the widget was "activated" (e.g. button clicked)
    virtual bool handleEvent(const SDL_Event *event, bool &isOver) = 0;
};

class UITextureButton : public UIWidget
{
public:
    SDL_Texture *texture = nullptr;
    SDL_Texture *pressedTexture = nullptr;
    bool isPressed = false;

    UITextureButton() = default;
    ~UITextureButton();

    // Loads textures directly from file paths.
    bool load(SDL_Renderer *renderer, const std::string &path, const std::string &pressedPath = "");

    // Safely destroys owned textures
    void cleanup();

    // Updates the position and size of the button
    void updateLayout(float x, float y, float w, float h) override;

    // Processes mouse events.
    // Returns true only if the mouse was pressed AND released over the button.
    // 'isOver' is an output parameter used to block clicks from hitting the board underneath.
    bool handleEvent(const SDL_Event *event, bool &isOver) override;

    // Renders the button.
    // Automatically handles pressed states and dims the button if 'enabled' is false.
    void render(SDL_Renderer *renderer) override;
};

class UILabel : public UIWidget
{
public:
    SDL_Texture *texture = nullptr;

    UILabel() = default;
    ~UILabel();

    // Creates a texture from text using the provided font and color.
    bool load(SDL_Renderer *renderer, TTF_Font *font, const std::string &text, SDL_Color color);

    // Safely destroys the texture
    void cleanup();

    // Updates the position and size of the label
    void updateLayout(float x, float y, float w, float h) override;

    // Labels are non-interactive by default, but must implement this to not be abstract
    bool handleEvent(const SDL_Event *event, bool &isOver) override;

    // Renders the label to the screen
    void render(SDL_Renderer *renderer) override;
};

class UIWorkingIndicator : public UIWidget
{
public:
    bool active = false;

    UIWorkingIndicator() = default;

    bool handleEvent(const SDL_Event *event, bool &isOver) override
    {
        isOver = false;
        return false;
    }
    void updateLayout(float x, float y, float w, float h) override;
    void render(SDL_Renderer *renderer) override;
};

class UIRadioButton : public UIWidget
{
public:
    UILabel label;
    bool selected = false;
    int value = 0;

    UIRadioButton() = default;
    void updateLayout(float x, float y, float w, float h) override;
    bool handleEvent(const SDL_Event *event, bool &isOver) override;
    void render(SDL_Renderer *renderer) override;
};

class UIModalDialogBox : public UIWidget
{
public:
    SDL_Color bgColor = {40, 40, 40, 255};
    bool visible = false;
    std::vector<UIWidget *> children;

    UIModalDialogBox() = default;
    virtual ~UIModalDialogBox() override = default;

    void addChild(UIWidget *child) { children.push_back(child); }

    // Positions the modal box
    void updateLayout(float centerX, float centerY, float width, float height) override;

    // Renders the background dimmer and the modal frame
    void render(SDL_Renderer *renderer) override;

    // Handles events for all children if visible
    bool handleEvent(const SDL_Event *event, bool &isOver) override;
};

#endif
