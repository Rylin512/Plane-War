#pragma once
#include <windows.h>
#include <gdiplus.h>
#include "Config.h"
#include "GameObject.h"

class Game;

enum class EnemyType { NORMAL, FAST, SHOOTING, BOSS };
enum class MovePattern { STRAIGHT, SINE, TRACKING, ZIGZAG, SPIRAL, CIRCLE };

// ============================================================
// 文件：Enemy.h
// 功能：敌机基类 + 三种子类 — GDI+ 渲染
// ============================================================

class Enemy : public GameObject {
public:
    Enemy(float x, float y, int w, int h, EnemyType type);
    virtual ~Enemy() {}

    virtual void update() override;
    virtual void render(Renderer& r) const override;

    virtual bool shouldShoot(int frameCount);
    virtual void resetShootTimer();

    EnemyType getType() const { return type; }

    EnemyType type;
    MovePattern movePattern;
    float speed;
    int hp, maxHp, scoreValue;
    int shootTimer, shootInterval;
    float sinePhase, sineAmplitude;
    int animFrame;
};

class NormalEnemy : public Enemy {
public:
    NormalEnemy(float startX);
};

class FastEnemy : public Enemy {
public:
    FastEnemy(float startX);
};

class ShootingEnemy : public Enemy {
public:
    ShootingEnemy(float startX);
    bool shouldShoot(int frameCount) override;
};
