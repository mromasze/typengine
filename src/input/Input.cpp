#include "Typengine/input/Input.h"
#include <SDL2/SDL.h>

namespace Typengine::Input {

bool Input::IsKeyPressed(Key k) {
    const Uint8* state = SDL_GetKeyboardState(NULL);
    SDL_Scancode sc = SDL_SCANCODE_UNKNOWN;

    switch (k) {
        case Key::W: sc = SDL_SCANCODE_W; break;
        case Key::A: sc = SDL_SCANCODE_A; break;
        case Key::S: sc = SDL_SCANCODE_S; break;
        case Key::D: sc = SDL_SCANCODE_D; break;
        case Key::SHIFT: sc = SDL_SCANCODE_LSHIFT; break;
        case Key::ESC: sc = SDL_SCANCODE_ESCAPE; break;
        default: break;
    }

    if (sc != SDL_SCANCODE_UNKNOWN) return state[sc];
    return false;
}

void Input::GetMousePos(int& x, int& y) {
    x = mouseX;
    y = mouseY;
}

bool Input::IsMouseButtonPressed(int button) {
    return (mouseState & SDL_BUTTON(button)) != 0;
}

void Input::Update() {
    mouseState = SDL_GetMouseState(&mouseX, &mouseY);
}

} 
