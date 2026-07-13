#include "engine/Input.hpp"

#include <SDL2/SDL.h>

namespace ty {

namespace {

SDL_Scancode toScancode(Key k) {
    switch (k) {
        case Key::W: return SDL_SCANCODE_W;
        case Key::A: return SDL_SCANCODE_A;
        case Key::S: return SDL_SCANCODE_S;
        case Key::D: return SDL_SCANCODE_D;
        case Key::Up: return SDL_SCANCODE_UP;
        case Key::Down: return SDL_SCANCODE_DOWN;
        case Key::Left: return SDL_SCANCODE_LEFT;
        case Key::Right: return SDL_SCANCODE_RIGHT;
        case Key::Shift: return SDL_SCANCODE_LSHIFT;
        case Key::Ctrl: return SDL_SCANCODE_LCTRL;
        case Key::Space: return SDL_SCANCODE_SPACE;
        case Key::Escape: return SDL_SCANCODE_ESCAPE;
        case Key::Enter: return SDL_SCANCODE_RETURN;
        case Key::Tab: return SDL_SCANCODE_TAB;
        case Key::E: return SDL_SCANCODE_E;
        case Key::Q: return SDL_SCANCODE_Q;
        case Key::F: return SDL_SCANCODE_F;
        case Key::R: return SDL_SCANCODE_R;
        case Key::I: return SDL_SCANCODE_I;
        case Key::Num1: return SDL_SCANCODE_1;
        case Key::Num2: return SDL_SCANCODE_2;
        case Key::Num3: return SDL_SCANCODE_3;
        case Key::Num4: return SDL_SCANCODE_4;
        case Key::Num5: return SDL_SCANCODE_5;
        default: return SDL_SCANCODE_UNKNOWN;
    }
}

} // namespace

void Input::update() {
    previous = current;
    prevMouseState = mouseState;

    const Uint8* state = SDL_GetKeyboardState(nullptr);
    for (int i = 0; i < KeyCount; ++i) {
        SDL_Scancode sc = toScancode(static_cast<Key>(i));
        current[i] = sc != SDL_SCANCODE_UNKNOWN && state[sc] != 0;
    }
    mouseState = SDL_GetMouseState(&mouseX, &mouseY);
}

bool Input::isDown(Key k) const { return current[static_cast<int>(k)]; }

bool Input::wasPressed(Key k) const {
    int i = static_cast<int>(k);
    return current[i] && !previous[i];
}

bool Input::wasReleased(Key k) const {
    int i = static_cast<int>(k);
    return !current[i] && previous[i];
}

bool Input::isMouseDown(MouseButton b) const {
    return (mouseState & SDL_BUTTON(static_cast<int>(b))) != 0;
}

bool Input::wasMousePressed(MouseButton b) const {
    Uint32 mask = SDL_BUTTON(static_cast<int>(b));
    return (mouseState & mask) != 0 && (prevMouseState & mask) == 0;
}

bool Input::wasMouseReleased(MouseButton b) const {
    Uint32 mask = SDL_BUTTON(static_cast<int>(b));
    return (mouseState & mask) == 0 && (prevMouseState & mask) != 0;
}

} // namespace ty
