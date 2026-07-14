#include "Player.h"
#include <cmath>

// ============================================================
// 文件：Player.cpp
// 功能：玩家飞机实现
// ============================================================

Player::Player()
    : GameObject(WINDOW_WIDTH / 2.0f - PLAYER_WIDTH / 2.0f,
                 WINDOW_HEIGHT - 80.0f,
                 PLAYER_WIDTH, PLAYER_HEIGHT)
    , lives(PLAYER_INIT_LIVES)
    , firepowerLevel(1)
    , firepowerTimer(0)
    , invincibleTimer(0)
    , shootCooldown(0)
    , bombCount(0)
    , shieldActive(false)
    , shieldTimer(0)
    , blinkCounter(0)
    , speedX(0), speedY(0)
{
}

void Player::update() {
    // 移动
    x += speedX;
    y += speedY;
    clampToScreen();

    // 射击冷却递减
    if (shootCooldown > 0) shootCooldown--;

    // 无敌计时器递减
    if (invincibleTimer > 0) {
        invincibleTimer--;
        blinkCounter++;
    }

    // 护盾计时器递减
    if (shieldActive) {
        shieldTimer--;
        if (shieldTimer <= 0) {
            shieldActive = false;
        }
    }

    // 火力增强计时器递减
    if (firepowerTimer > 0) {
        firepowerTimer--;
        if (firepowerTimer <= 0 && firepowerLevel > 1) {
            firepowerLevel = 1;  // 恢复到基础火力
        }
    }
}

void Player::move(float dx, float dy) {
    x += dx;
    y += dy;
    clampToScreen();
}

void Player::clampToScreen() {
    x = clamp(x, 0.0f, (float)(WINDOW_WIDTH - width));
    y = clamp(y, 0.0f, (float)(WINDOW_HEIGHT - height));
}

bool Player::canShoot() const {
    return shootCooldown <= 0;
}

void Player::resetShootCooldown() {
    shootCooldown = PLAYER_SHOOT_COOLDOWN;
}

void Player::takeDamage() {
    if (isInvincible()) return;

    if (shieldActive) {
        // 护盾吸收伤害
        shieldActive = false;
        shieldTimer = 0;
        return;
    }

    lives--;
    invincibleTimer = PLAYER_INVINCIBLE_FRAMES;
    blinkCounter = 0;

    if (lives < 0) lives = 0;
}

bool Player::isInvincible() const {
    return invincibleTimer > 0;
}

void Player::addLife() {
    if (lives < PLAYER_MAX_LIVES) lives++;
}

void Player::addBomb() {
    if (bombCount < PLAYER_MAX_BOMBS) bombCount++;
}

void Player::activateShield() {
    shieldActive = true;
    shieldTimer = SHIELD_DURATION;
}

void Player::increaseFirepower() {
    if (firepowerLevel < PLAYER_MAX_FIREPOWER) {
        firepowerLevel++;
    }
    firepowerTimer = FIREPOWER_DURATION;  // 刷新持续时间
}

void Player::render(HDC hdc) const {
    // 无敌闪烁：每6帧切换显示/隐藏
    if (isInvincible() && (blinkCounter / 6) % 2 == 0) {
        return;  // 闪烁时跳过绘制
    }

    float cx = x + width / 2.0f;
    float cy = y + height / 2.0f;

    // 护盾光晕
    if (shieldActive) {
        HPEN shieldPen = CreatePen(PS_SOLID, 2, RGB(0, 180, 255));
        HBRUSH shieldBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
        HPEN oldPen = (HPEN)SelectObject(hdc, shieldPen);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, shieldBrush);
        Ellipse(hdc, (int)(cx - 25), (int)(cy - 25), (int)(cx + 25), (int)(cy + 25));
        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBrush);
        DeleteObject(shieldPen);
    }

    // 飞机颜色（半透明效果简化处理）
    COLORREF bodyColor = RGB(30, 180, 255);    // 蓝色机身
    COLORREF wingColor = RGB(200, 200, 200);   // 灰色机翼
    COLORREF cockpitColor = RGB(0, 255, 255);  // 青色驾驶舱
    COLORREF engineColor = RGB(255, 150, 0);   // 橙色引擎

    // ---- 绘制机身（三角形） ----
    HPEN bodyPen = CreatePen(PS_SOLID, 1, bodyColor);
    HBRUSH bodyBrush = CreateSolidBrush(bodyColor);
    HPEN oldPen = (HPEN)SelectObject(hdc, bodyPen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, bodyBrush);

    // 机身多边形：从顶部尖端到底部
    POINT body[5] = {
        { (int)(cx),       (int)(cy - height / 2) },          // 机头尖端
        { (int)(cx - 8),   (int)(cy + height / 2 - 8) },      // 左下
        { (int)(cx - 4),   (int)(cy + height / 2) },          // 底部左
        { (int)(cx + 4),   (int)(cy + height / 2) },          // 底部右
        { (int)(cx + 8),   (int)(cy + height / 2 - 8) },      // 右下
    };
    Polygon(hdc, body, 5);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(bodyBrush);
    DeleteObject(bodyPen);

    // ---- 绘制主翼 ----
    HPEN wingPen = CreatePen(PS_SOLID, 2, wingColor);
    oldPen = (HPEN)SelectObject(hdc, wingPen);
    MoveToEx(hdc, (int)(cx - 20), (int)(cy - 4), nullptr);
    LineTo(hdc, (int)(cx + 20), (int)(cy - 4));
    SelectObject(hdc, oldPen);
    DeleteObject(wingPen);

    // ---- 绘制尾翼 ----
    HPEN tailPen = CreatePen(PS_SOLID, 1, wingColor);
    oldPen = (HPEN)SelectObject(hdc, tailPen);
    MoveToEx(hdc, (int)(cx - 14), (int)(cy + height / 2 - 8), nullptr);
    LineTo(hdc, (int)(cx + 14), (int)(cy + height / 2 - 8));
    SelectObject(hdc, oldPen);
    DeleteObject(tailPen);

    // ---- 绘制驾驶舱 ----
    HBRUSH cockpitBrush = CreateSolidBrush(cockpitColor);
    oldBrush = (HBRUSH)SelectObject(hdc, cockpitBrush);
    Ellipse(hdc, (int)(cx - 4), (int)(cy - 8), (int)(cx + 4), (int)(cy));
    SelectObject(hdc, oldBrush);
    DeleteObject(cockpitBrush);

    // ---- 引擎火焰 ----
    if (speedY < 0 || (speedX != 0)) {
        // 移动时绘制引擎火焰
        HPEN enginePen = CreatePen(PS_SOLID, 1, engineColor);
        oldPen = (HPEN)SelectObject(hdc, enginePen);
        // 左引擎火焰
        MoveToEx(hdc, (int)(cx - 5), (int)(cy + height / 2), nullptr);
        LineTo(hdc, (int)(cx - 5), (int)(cy + height / 2 + 6));
        // 右引擎火焰
        MoveToEx(hdc, (int)(cx + 5), (int)(cy + height / 2), nullptr);
        LineTo(hdc, (int)(cx + 5), (int)(cy + height / 2 + 6));
        SelectObject(hdc, oldPen);
        DeleteObject(enginePen);
    }
}
