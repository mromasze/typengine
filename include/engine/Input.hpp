#pragma once
// Typengine — Input.hpp
// Keyboard/mouse polling with per-frame edge detection
// (isDown = held, wasPressed/wasReleased = this frame only).

#include <array>
#include <cstdint>

#include "engine/Math.hpp"

namespace ty {

enum class Key {
    W, A, S, D,
    Up, Down, Left, Right,
    Shift, Ctrl, Space, Escape, Enter, Tab,
    E, Q, F, R, I,
    Num1, Num2, Num3, Num4, Num5,
    Count
};

enum class MouseButton { Left = 1, Middle = 2, Right = 3 };

class Input {
public:
    // Snapshot device state; call once per frame (Engine does this for you).
    void update();

    bool isDown(Key k) const;
    bool wasPressed(Key k) const;
    bool wasReleased(Key k) const;

    Vec2i mousePosition() const { return {mouseX, mouseY}; }
    bool isMouseDown(MouseButton b) const;
    bool wasMousePressed(MouseButton b) const;
    bool wasMouseReleased(MouseButton b) const;

private:
    static constexpr int KeyCount = static_cast<int>(Key::Count);
    std::array<bool, KeyCount> current{};
    std::array<bool, KeyCount> previous{};
    int mouseX = 0;
    int mouseY = 0;
    std::uint32_t mouseState = 0;
    std::uint32_t prevMouseState = 0;
};

} // namespace ty
