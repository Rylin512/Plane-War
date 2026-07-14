#pragma once
#include <windows.h>
#include "Config.h"
#include "GameObject.h"

// 前向声明
class Game;

// ============================================================
// 文件：Player.h
// 功能：玩家飞机类 — 移动、射击、生命值、道具状态
// ============================================================

class Player : public GameObject {
public:
    Player();

    void update() override;

    // 移动
    void move(float dx, float dy);
    void clampToScreen();

    // 射击
    bool canShoot() const;
    void resetShootCooldown();

    // 受击
    void takeDamage();
    bool isInvincible() const;

    // 道具状态
    void addLife();
    void addBomb();
    void activateShield();
    void increaseFirepower();

    // 渲染
    void render(HDC hdc) const;

    // ---- 状态 ----
    int lives;
    int firepowerLevel;
    int firepowerTimer;     // 火力增强剩余帧数
    int invincibleTimer;
    int shootCooldown;
    int bombCount;
    bool shieldActive;
    int shieldTimer;
    int blinkCounter;

    // 移动速度
    float speedX, speedY;
};
