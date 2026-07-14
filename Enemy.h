#pragma once
#include <windows.h>
#include "Config.h"
#include "GameObject.h"

class Game;

// ============================================================
// 文件：Enemy.h
// 功能：敌机类层级 — 普通敌机、快速敌机、射击敌机
// ============================================================

// 敌机类型
enum class EnemyType { NORMAL, FAST, SHOOTING, BOSS };

// 移动模式
enum class MovePattern { STRAIGHT, SINE, TRACKING };

class Enemy : public GameObject {
public:
    Enemy(float x, float y, int w, int h, EnemyType type);
    virtual ~Enemy() {}

    virtual void update() override;
    virtual void render(HDC hdc) const;

    // 射击逻辑（射击敌机/Boss重写）
    virtual bool shouldShoot(int frameCount);
    virtual void resetShootTimer();

    // 获取类型
    EnemyType getType() const { return type; }

    // ---- 属性 ----
    EnemyType type;
    MovePattern movePattern;
    float speed;
    int hp;
    int maxHp;
    int scoreValue;
    int shootTimer;         // 射击间隔计时器
    int shootInterval;      // 射击间隔帧数
    float sinePhase;        // 正弦波相位
    float sineAmplitude;    // 正弦波振幅
    int animFrame;          // 动画帧
};

// 普通敌机 — 直线下降
class NormalEnemy : public Enemy {
public:
    NormalEnemy(float startX);
};

// 快速敌机 — 小体型、快速下降
class FastEnemy : public Enemy {
public:
    FastEnemy(float startX);
};

// 射击敌机 — 缓慢下降、向下开火
class ShootingEnemy : public Enemy {
public:
    ShootingEnemy(float startX);
    bool shouldShoot(int frameCount) override;
};
