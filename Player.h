#pragma once
#include <windows.h>
#include <gdiplus.h>
#include "Config.h"
#include "GameObject.h"

class Game;

// ============================================================
// 文件：Player.h
// 功能：玩家飞机 — 变速移动 (Shift/Ctrl) + GDI+ 渲染
// ============================================================

class Player : public GameObject {
public:
    Player();

    void update() override;
    void render(Renderer& r) const override;

    void move(float dx, float dy);
    void clampToScreen();

    bool canShoot() const;
    void resetShootCooldown();

    void takeDamage();
    bool isInvincible() const;

    void addLife();
    void addBomb();
    void activateShield();
    void increaseFirepower();
    void boostFireRate();

    // ── 变速方法 ──
    void setFastMode(bool on);    // Shift
    void setSlowMode(bool on);    // Ctrl
    float getCurrentSpeed() const;

    // ── 状态 ──
    int lives, firepowerLevel, firepowerTimer;
    int invincibleTimer, shootCooldown;
    int bombCount;
    bool shieldActive; int shieldTimer;
    bool fireRateBoost; int fireRateTimer;
    int blinkCounter;
    float speedX, speedY;

    // 变速状态
    bool fastMode, slowMode;
    float currentSpeedMult;
    void updateSpeedMult();
};
