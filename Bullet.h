#pragma once
#include <windows.h>
#include <gdiplus.h>
#include "Config.h"
#include "GameObject.h"

enum class BulletOwner { PLAYER, ENEMY };

// ============================================================
// 文件：Bullet.h
// 功能：子弹 — GDI+ 发光渲染
// ============================================================

class Bullet : public GameObject {
public:
    Bullet(float x, float y, int w, int h, BulletOwner owner, float speedY);
    virtual ~Bullet() {}
    virtual void update() override;
    virtual void render(Renderer& r) const override;

    BulletOwner getOwner() const { return owner; }

    BulletOwner owner;
    float speedY;
    int damage;
};

class PlayerBullet : public Bullet {
public:
    PlayerBullet(float x, float y);
    void reset(float nx, float ny);
};

class EnemyBullet : public Bullet {
public:
    EnemyBullet(float x, float y, float angleX = 0);
    void reset(float nx, float ny, float nAngleX = 0);
    void update() override;
    void render(Renderer& r) const override;
    float speedX;
};
