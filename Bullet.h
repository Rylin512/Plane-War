#pragma once
#include <windows.h>
#include "Config.h"
#include "GameObject.h"

// ============================================================
// 文件：Bullet.h
// 功能：子弹类 — 玩家子弹和敌机子弹
// ============================================================

enum class BulletOwner { PLAYER, ENEMY };

class Bullet : public GameObject {
public:
    Bullet(float x, float y, int w, int h, BulletOwner owner, float speedY);
    virtual ~Bullet() {}

    virtual void update() override;
    virtual void render(HDC hdc) const;

    BulletOwner owner;
    float speedY;
    int damage;
};

// 玩家子弹 — 向上飞行
class PlayerBullet : public Bullet {
public:
    PlayerBullet(float x, float y);
};

// 敌机子弹 — 向下飞行，可带角度
class EnemyBullet : public Bullet {
public:
    EnemyBullet(float x, float y, float angleX = 0.0f);
    void update() override;
    void render(HDC hdc) const override;

    float speedX;  // 水平速度分量（追踪子弹用）
};
