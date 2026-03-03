#pragma once
#include <cmath>

struct Vec2 {
    float x, y;
    
    Vec2 operator+(const Vec2& other) const { return {x + other.x, y + other.y}; }
    Vec2 operator-(const Vec2& other) const { return {x - other.x, y - other.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    
    float length() const { return std::sqrt(x*x + y*y); }
    Vec2 normalized() const {
        float l = length();
        if (l == 0) return {0,0};
        return {x/l, y/l};
    }
};

struct Rect {
    float x, y, w, h;
};
