#pragma once
#include <cmath>

// ============================================================
// 文件：GameObject.h
// 功能：所有游戏实体的抽象基类
// ============================================================

struct GameObject {
    float x, y;          // 位置（左上角坐标）
    int width, height;   // 碰撞盒尺寸
    bool alive;           // 是否存活

    GameObject(float _x = 0, float _y = 0, int _w = 0, int _h = 0)
        : x(_x), y(_y), width(_w), height(_h), alive(true) {}

    virtual ~GameObject() {}

    // 获取中心点坐标
    float centerX() const { return x + width / 2.0f; }
    float centerY() const { return y + height / 2.0f; }

    // 更新逻辑
    virtual void update() = 0;
};

// AABB 碰撞检测（中心点距离法）
inline bool checkCollision(const GameObject& a, const GameObject& b) {
    if (!a.alive || !b.alive) return false;
    return (std::abs(a.centerX() - b.centerX()) < (a.width + b.width) / 2.0f) &&
           (std::abs(a.centerY() - b.centerY()) < (a.height + b.height) / 2.0f);
}

// 数值钳制
inline float clamp(float val, float lo, float hi) {
    if (val < lo) return lo;
    if (val > hi) return hi;
    return val;
}
