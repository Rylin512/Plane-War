#include "Bullet.h"

// ============================================================
// 文件：Bullet.cpp
// 功能：子弹类实现
// ============================================================

Bullet::Bullet(float x, float y, int w, int h, BulletOwner _owner, float _speedY)
    : GameObject(x, y, w, h)
    , owner(_owner), speedY(_speedY), damage(1)
{
}

void Bullet::update() {
    y += speedY;
}

void Bullet::render(HDC hdc) const {
    HBRUSH brush;
    HPEN pen;

    if (owner == BulletOwner::PLAYER) {
        // 玩家子弹 — 亮黄色
        brush = CreateSolidBrush(RGB(255, 255, 50));
        pen = CreatePen(PS_SOLID, 1, RGB(255, 255, 100));
    } else {
        // 敌机子弹 — 红色圆形
        brush = CreateSolidBrush(RGB(255, 60, 60));
        pen = CreatePen(PS_SOLID, 1, RGB(255, 100, 100));
    }

    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);

    if (owner == BulletOwner::PLAYER) {
        // 玩家子弹：细长椭圆形
        Ellipse(hdc, (int)x, (int)y, (int)(x + width), (int)(y + height));
    } else {
        // 敌机子弹：圆形
        Ellipse(hdc, (int)x, (int)y, (int)(x + width), (int)(y + height));
    }

    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
}

// ======== PlayerBullet ========

PlayerBullet::PlayerBullet(float x, float y)
    : Bullet(x, y, BULLET_PLAYER_WIDTH, BULLET_PLAYER_HEIGHT,
             BulletOwner::PLAYER, -BULLET_PLAYER_SPEED)
{
    damage = 1;
}

// ======== EnemyBullet ========

EnemyBullet::EnemyBullet(float x, float y, float angleX)
    : Bullet(x, y, BULLET_ENEMY_WIDTH, BULLET_ENEMY_HEIGHT,
             BulletOwner::ENEMY, BULLET_ENEMY_SPEED)
    , speedX(angleX)
{
    damage = 1;
}

void EnemyBullet::update() {
    x += speedX;
    y += speedY;
}

void EnemyBullet::render(HDC hdc) const {
    HBRUSH brush = CreateSolidBrush(RGB(255, 60, 60));
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(255, 100, 100));
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);

    Ellipse(hdc, (int)x, (int)y, (int)(x + width), (int)(y + height));

    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
}
