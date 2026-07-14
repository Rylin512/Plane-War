#pragma once
#include <cmath>
#include <gdiplus.h>

// ============================================================
// 文件：GameObject.h
// 功能：实体基类 + AABB 碰撞 + 工具函数
// ============================================================

struct GameObject {
    float x, y;
    int width, height;
    bool alive;

    GameObject(float _x = 0, float _y = 0, int _w = 0, int _h = 0)
        : x(_x), y(_y), width(_w), height(_h), alive(true) {}

    virtual ~GameObject() {}

    float centerX() const { return x + width  / 2.0f; }
    float centerY() const { return y + height / 2.0f; }

    virtual void update() = 0;
    virtual void render(Gdiplus::Graphics& g) const = 0;
};

// ── AABB 碰撞 ──
inline bool checkCollision(const GameObject& a, const GameObject& b) {
    if (!a.alive || !b.alive) return false;
    return (std::abs(a.centerX() - b.centerX()) < (a.width  + b.width)  / 2.0f) &&
           (std::abs(a.centerY() - b.centerY()) < (a.height + b.height) / 2.0f);
}

inline float clamp(float v, float lo, float hi) {
    if (v < lo) return lo; if (v > hi) return hi; return v;
}

// ── 浮点数 lerp ──
inline float lerp(float a, float b, float t) { return a + (b - a) * t; }
