#include "uiWidgets.h"
#include <SDL3_image/SDL_image.h>

UITextureButton::~UITextureButton()
{
    cleanup();
}

bool UITextureButton::load(SDL_Renderer *renderer, const std::string &path, const std::string &pressedPath)
{
    cleanup(); // Clear existing if re-loading

    texture = IMG_LoadTexture(renderer, path.c_str());
    if (texture)
        SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);

    if (!pressedPath.empty())
    {
        pressedTexture = IMG_LoadTexture(renderer, pressedPath.c_str());
        if (pressedTexture)
            SDL_SetTextureScaleMode(pressedTexture, SDL_SCALEMODE_LINEAR);
    }

    return (texture != nullptr);
}

void UITextureButton::cleanup()
{
    if (texture)
    {
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }
    if (pressedTexture)
    {
        SDL_DestroyTexture(pressedTexture);
        pressedTexture = nullptr;
    }
}

void UITextureButton::updateLayout(float x, float y, float w, float h)
{
    rect = {x, y, w, h};
}

bool UITextureButton::handleEvent(const SDL_Event *event, bool &isOver)
{
    bool isMouseEvent = (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
                         event->type == SDL_EVENT_MOUSE_BUTTON_UP ||
                         event->type == SDL_EVENT_MOUSE_MOTION);

    if (!isMouseEvent)
        return false;

    float mx, my;
    if (event->type == SDL_EVENT_MOUSE_MOTION)
    {
        mx = event->motion.x;
        my = event->motion.y;
    }
    else
    {
        mx = event->button.x;
        my = event->button.y;
    }

    // Increase buffer to 2.0f to handle high-DPI mouse jitter during fast clicks
    isOver = (mx >= rect.x - 2.0f && mx <= rect.x + rect.w + 2.0f &&
              my >= rect.y - 2.0f && my <= rect.y + rect.h + 2.0f);

    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
        if (isOver && enabled)
            isPressed = true;
    }
    else if (event->type == SDL_EVENT_MOUSE_BUTTON_UP)
    {
        bool clicked = isPressed && isOver && enabled;
        isPressed = false;
        return clicked;
    }

    if (!enabled && isPressed)
        isPressed = false;
    return false;
}

void UITextureButton::render(SDL_Renderer *renderer)
{
    SDL_Texture *currentTex = (isPressed && pressedTexture) ? pressedTexture : texture;
    if (!currentTex)
        return;

    if (!enabled)
    {
        // Dim the button when disabled
        SDL_SetTextureColorMod(currentTex, 100, 100, 100);
        SDL_SetTextureAlphaMod(currentTex, 150);
    }
    else
    {
        SDL_SetTextureColorMod(currentTex, 255, 255, 255);
        SDL_SetTextureAlphaMod(currentTex, 255);
    }

    SDL_RenderTexture(renderer, currentTex, NULL, &rect);
}
