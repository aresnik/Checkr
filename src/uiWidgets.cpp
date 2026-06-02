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
    isOver = false;
    bool isMouseEvent = (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
                         event->type == SDL_EVENT_MOUSE_BUTTON_UP ||
                         event->type == SDL_EVENT_MOUSE_MOTION);

    if (!isMouseEvent)
        return false;

    float mx = 0, my = 0;
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

UILabel::~UILabel()
{
    cleanup();
}

bool UILabel::load(SDL_Renderer *renderer, TTF_Font *font, const std::string &text, SDL_Color color)
{
    cleanup();
    if (!font || text.empty())
        return false;

    SDL_Surface *surface = TTF_RenderText_Blended(font, text.c_str(), text.length(), color);
    if (!surface)
        return false;

    texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture)
    {
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);
    }

    SDL_DestroySurface(surface);
    return (texture != nullptr);
}

void UILabel::cleanup()
{
    if (texture)
    {
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }
}

void UILabel::updateLayout(float x, float y, float w, float h)
{
    rect = {x, y, w, h};
}

bool UILabel::handleEvent(const SDL_Event *event, bool &isOver)
{
    isOver = false;
    return false;
}

void UILabel::render(SDL_Renderer *renderer)
{
    if (texture)
    {
        SDL_RenderTexture(renderer, texture, NULL, &rect);
    }
}

void UIWorkingIndicator::updateLayout(float x, float y, float w, float h)
{
    rect = {x, y, w, h};
}

void UIWorkingIndicator::render(SDL_Renderer *renderer)
{
    if (!active)
        return;

    // Enable blending for the translucent box
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_RenderFillRect(renderer, &rect);

    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderRect(renderer, &rect);

    Uint32 ticks = SDL_GetTicks();
    int dotCount = (ticks / 300) % 7;

    for (int i = 0; i < dotCount; i++)
    {
        SDL_FRect dot;
        dot.x = rect.x + 10 + i * 12;
        dot.y = rect.y + (rect.h / 2) - 2;
        dot.w = 4;
        dot.h = 4;
        SDL_RenderFillRect(renderer, &dot);
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void UIRadioButton::updateLayout(float x, float y, float w, float h)
{
    rect = {x, y, w, h};
    label.updateLayout(x + w + 10, y, w * 4, h);
}

bool UIRadioButton::handleEvent(const SDL_Event *event, bool &isOver)
{
    isOver = false;
    float mx = 0, my = 0;
    bool isMouseEvent = (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
                         event->type == SDL_EVENT_MOUSE_BUTTON_UP ||
                         event->type == SDL_EVENT_MOUSE_MOTION);

    if (!isMouseEvent)
        return false;

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

    // Use consistent 2.0f buffer and include the label area (+150) in the hit-test
    isOver = (mx >= rect.x - 2.0f && mx <= rect.x + rect.w + 150.0f &&
              my >= rect.y - 2.0f && my <= rect.y + rect.h + 2.0f);

    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && isOver)
    {
        return true;
    }
    return false;
}

void UIRadioButton::render(SDL_Renderer *renderer)
{
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderRect(renderer, &rect);

    if (selected)
    {
        SDL_FRect inner = {rect.x + 5, rect.y + 5, rect.w - 10, rect.h - 10};
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_RenderFillRect(renderer, &inner);
    }
    label.render(renderer);
}

void UIModalDialogBox::updateLayout(float centerX, float centerY, float width, float height)
{
    rect = {centerX - width / 2, centerY - height / 2, width, height};
}

void UIModalDialogBox::render(SDL_Renderer *renderer)
{
    if (!visible)
        return;

    // Background Dimmer: Query actual logical size to avoid hardcoding
    int w, h;
    SDL_GetRenderLogicalPresentation(renderer, &w, &h, NULL);
    SDL_FRect screen = {0, 0, (float)w, (float)h};

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_RenderFillRect(renderer, &screen);

    // Modal Box Frame (Generic Modal Behavior)
    SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
    SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderRect(renderer, &rect);

    // Render all children (Composition-based)
    for (auto *child : children)
    {
        child->render(renderer);
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

bool UIModalDialogBox::handleEvent(const SDL_Event *event, bool &isOver)
{
    isOver = false;
    if (!visible)
        return false;

    float mx = 0, my = 0;
    if (event->type == SDL_EVENT_MOUSE_MOTION)
    {
        mx = event->motion.x;
        my = event->motion.y;
    }
    else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN || event->type == SDL_EVENT_MOUSE_BUTTON_UP)
    {
        mx = event->button.x;
        my = event->button.y;
    }
    else
    {
        return false;
    }

    // Block input if mouse is inside the modal frame
    isOver = (mx >= rect.x && mx <= rect.x + rect.w && my >= rect.y && my <= rect.y + rect.h);

    bool activated = false;
    for (auto *child : children)
    {
        bool dummy;
        if (child->handleEvent(event, dummy))
            activated = true;
    }
    return activated;
}
