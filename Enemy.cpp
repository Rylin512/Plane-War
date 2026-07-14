#include "Enemy.h"
#include <cmath>

// ============================================================
// 文件：Enemy.cpp
// 功能：敌机类实现
// ============================================================

Enemy::Enemy(float x, float y, int w, int h, EnemyType _type)
    : GameObject(x, y, w, h)
    , type(_type)
    , movePattern(MovePattern::STRAIGHT)
    , speed(1.0f)
    , hp(1), maxHp(1)
    , scoreValue(10)
    , shootTimer(0), shootInterval(60)
    , sinePhase(0.0f), sineAmplitude(0.0f)
    , animFrame(0)
{
}

void Enemy::update() {
    animFrame++;

    // 正弦波移动
    if (movePattern == MovePattern::SINE) {
        x += sin((float)animFrame * 0.08f + sinePhase) * sineAmplitude;
    }

    // 追踪移动
    if (movePattern == MovePattern::TRACKING) {
        // 由外部设置 speedX 朝向玩家
    }

    // 向下移动
    y += speed;

    // 射击计时器
    if (shootTimer > 0) shootTimer--;
}

void Enemy::render(HDC hdc) const {
    float cx = x + width / 2.0f;
    float cy = y + height / 2.0f;

    switch (type) {
    case EnemyType::NORMAL: {
        // 普通敌机 — 红色菱形机身
        COLORREF bodyColor = RGB(220, 50, 50);
        HBRUSH brush = CreateSolidBrush(bodyColor);
        HPEN pen = CreatePen(PS_SOLID, 1, bodyColor);
        HPEN oldPen = (HPEN)SelectObject(hdc, pen);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);

        // 倒三角形机身
        POINT pts[5] = {
            { (int)(cx),             (int)(cy + height / 2) },     // 底部尖端
            { (int)(cx - width / 2), (int)(cy - height / 2) },     // 左上
            { (int)(cx - width / 4), (int)(cy - height / 4) },
            { (int)(cx + width / 4), (int)(cy - height / 4) },
            { (int)(cx + width / 2), (int)(cy - height / 2) },     // 右上
        };
        Polygon(hdc, pts, 5);
        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(brush);
        DeleteObject(pen);

        // 小翅膀
        HPEN wingPen = CreatePen(PS_SOLID, 1, RGB(180, 40, 40));
        oldPen = (HPEN)SelectObject(hdc, wingPen);
        MoveToEx(hdc, (int)(cx - width / 2), (int)(cy - height / 4), nullptr);
        LineTo(hdc, (int)(cx - width / 2 - 6), (int)(cy + 2));
        MoveToEx(hdc, (int)(cx + width / 2), (int)(cy - height / 4), nullptr);
        LineTo(hdc, (int)(cx + width / 2 + 6), (int)(cy + 2));
        SelectObject(hdc, oldPen);
        DeleteObject(wingPen);
        break;
    }
    case EnemyType::FAST: {
        // 快速敌机 — 黄色小型三角形
        COLORREF bodyColor = RGB(255, 180, 30);
        HBRUSH brush = CreateSolidBrush(bodyColor);
        HPEN pen = CreatePen(PS_SOLID, 1, bodyColor);
        HPEN oldPen = (HPEN)SelectObject(hdc, pen);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);

        POINT pts[3] = {
            { (int)(cx),             (int)(cy + height / 2) },
            { (int)(cx - width / 2), (int)(cy - height / 2) },
            { (int)(cx + width / 2), (int)(cy - height / 2) },
        };
        Polygon(hdc, pts, 3);
        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(brush);
        DeleteObject(pen);
        break;
    }
    case EnemyType::SHOOTING: {
        // 射击敌机 — 紫色八角形
        COLORREF bodyColor = RGB(180, 50, 220);
        HBRUSH brush = CreateSolidBrush(bodyColor);
        HPEN pen = CreatePen(PS_SOLID, 2, RGB(220, 100, 255));
        HPEN oldPen = (HPEN)SelectObject(hdc, pen);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);

        RoundRect(hdc, (int)x, (int)y, (int)(x + width), (int)(y + height), 8, 8);
        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(brush);
        DeleteObject(pen);

        // 炮管指示
        HPEN gunPen = CreatePen(PS_SOLID, 2, RGB(255, 80, 255));
        oldPen = (HPEN)SelectObject(hdc, gunPen);
        MoveToEx(hdc, (int)(cx), (int)(y + height), nullptr);
        LineTo(hdc, (int)(cx), (int)(y + height + 6));
        SelectObject(hdc, oldPen);
        DeleteObject(gunPen);
        break;
    }
    case EnemyType::BOSS:
        // Boss 渲染在 Boss.cpp 中单独处理
        break;
    }
}

bool Enemy::shouldShoot(int frameCount) {
    if (shootTimer > 0) return false;
    return (frameCount % shootInterval == 0);
}

void Enemy::resetShootTimer() {
    shootTimer = shootInterval;
}

// ======== NormalEnemy ========

NormalEnemy::NormalEnemy(float startX)
    : Enemy(startX, -(float)ENEMY_NORMAL_HEIGHT,
            ENEMY_NORMAL_WIDTH, ENEMY_NORMAL_HEIGHT,
            EnemyType::NORMAL)
{
    speed = ENEMY_NORMAL_SPEED;
    hp = 1; maxHp = 1;
    scoreValue = ENEMY_NORMAL_SCORE;
}

// ======== FastEnemy ========

FastEnemy::FastEnemy(float startX)
    : Enemy(startX, -(float)ENEMY_FAST_HEIGHT,
            ENEMY_FAST_WIDTH, ENEMY_FAST_HEIGHT,
            EnemyType::FAST)
{
    speed = ENEMY_FAST_SPEED;
    hp = 1; maxHp = 1;
    scoreValue = ENEMY_FAST_SCORE;
}

// ======== ShootingEnemy ========

ShootingEnemy::ShootingEnemy(float startX)
    : Enemy(startX, -(float)ENEMY_SHOOTING_HEIGHT,
            ENEMY_SHOOTING_WIDTH, ENEMY_SHOOTING_HEIGHT,
            EnemyType::SHOOTING)
{
    speed = ENEMY_SHOOTING_SPEED;
    hp = 2; maxHp = 2;
    scoreValue = ENEMY_SHOOTING_SCORE;
    shootInterval = 70;
    shootTimer = 30 + rand() % 40;  // 随机初始延迟
}

bool ShootingEnemy::shouldShoot(int frameCount) {
    if (shootTimer > 0) return false;
    return true;  // 计时器归零就射击
}
