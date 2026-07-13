#pragma once
// Typengine — Math.hpp
// Small self-contained math types used across the public API.

#include <cmath>
#include <cstdint>

namespace ty {

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    Vec2 operator/(float s) const { return {x / s, y / s}; }
    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    bool operator==(const Vec2& o) const { return x == o.x && y == o.y; }

    float length() const { return std::sqrt(x * x + y * y); }
    float lengthSq() const { return x * x + y * y; }

    Vec2 normalized() const {
        float l = length();
        if (l == 0.0f) return {0.0f, 0.0f};
        return {x / l, y / l};
    }

    static float dot(const Vec2& a, const Vec2& b) { return a.x * b.x + a.y * b.y; }
    static Vec2 lerp(const Vec2& a, const Vec2& b, float t) {
        return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
    }
};

struct Vec2i {
    int x = 0;
    int y = 0;
    bool operator==(const Vec2i& o) const { return x == o.x && y == o.y; }
};

struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

    bool intersects(const Rect& o) const {
        return x < o.x + o.w && x + w > o.x &&
               y < o.y + o.h && y + h > o.y;
    }
    bool contains(const Vec2& p) const {
        return p.x >= x && p.x <= x + w && p.y >= y && p.y <= y + h;
    }
    Vec2 center() const { return {x + w * 0.5f, y + h * 0.5f}; }
};

struct Color {
    std::uint8_t r = 255;
    std::uint8_t g = 255;
    std::uint8_t b = 255;
    std::uint8_t a = 255;
};

inline float clamp(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline float lerp(float a, float b, float t) { return a + (b - a) * t; }

// Frame-rate independent exponential smoothing factor.
// Usage: pos = Vec2::lerp(pos, target, smoothFactor(8.0f, dt));
inline float smoothFactor(float speed, float dt) { return 1.0f - std::exp(-speed * dt); }

} // namespace ty
