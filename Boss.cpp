#include "Boss.h"
#include "Game.h"
#include "Player.h"
#include "Bullet.h"
#include <cmath>

// ============================================================
// 文件：Boss.cpp
// 功能：Boss 敌机实现 — 多阶段 AI、弹幕模式、血条
// ============================================================

Boss::Boss(float startX, int level)
    : Enemy(startX, -(float)BOSS_HEIGHT, BOSS_WIDTH, BOSS_HEIGHT, EnemyType::BOSS)
    , phase(BossPhase::ENTRY)
    , phaseTimer(0), patternTimer(0), patternIndex(0)
    , targetX(WINDOW_WIDTH / 2.0f), entryTimer(60), entered(false)
    , animTimer(0), wingAngle(0)
{
    speed = BOSS_SPEED;
    hp = BOSS_BASE_HP + level * 10;
    maxHp = hp;
    scoreValue = BOSS_SCORE + level * 1000;
    type = EnemyType::BOSS;
}

BossPhase Boss::getCurrentPhase() const {
    return phase;
}

void Boss::updatePhase() {
    float hpPercent = (float)hp / maxHp;

    if (hpPercent > 0.66f) {
        phase = (phase != BossPhase::PHASE_1) ? BossPhase::PHASE_1 : phase;
    } else if (hpPercent > 0.33f) {
        if (phase != BossPhase::PHASE_2) {
            phase = BossPhase::PHASE_2;
            phaseTimer = 0;
            patternIndex = 0;
        }
    } else {
        if (phase != BossPhase::PHASE_3) {
            phase = BossPhase::PHASE_3;
            phaseTimer = 0;
            patternIndex = 0;
        }
    }
}

void Boss::update() {
    animTimer++;

    // 入场动画
    if (!entered) {
        entryTimer--;
        y += 1.5f;
        if (y >= 50.0f) {
            y = 50.0f;
            entered = true;
            phase = BossPhase::PHASE_1;
        }
        return;
    }

    // 更新阶段
    updatePhase();

    // 阶段行为
    switch (phase) {
    case BossPhase::PHASE_1:
        // 缓慢左右移动
        targetX = WINDOW_WIDTH / 2.0f + sinf((float)animTimer * 0.02f) * 150.0f;
        x += (targetX - x) * 0.02f;
        // 限制边界
        x = clamp(x, 0.0f, (float)(WINDOW_WIDTH - width));
        break;

    case BossPhase::PHASE_2:
        // 快速左右移动
        targetX = WINDOW_WIDTH / 2.0f + sinf((float)animTimer * 0.04f) * 200.0f;
        x += (targetX - x) * 0.05f;
        x = clamp(x, 0.0f, (float)(WINDOW_WIDTH - width));
        break;

    case BossPhase::PHASE_3:
        // 激烈移动 + 追踪玩家
        targetX = WINDOW_WIDTH / 2.0f + sinf((float)animTimer * 0.06f) * 180.0f;
        x += (targetX - x) * 0.08f;
        x = clamp(x, 0.0f, (float)(WINDOW_WIDTH - width));
        break;

    default: break;
    }

    // 弹幕计时器
    if (patternTimer > 0) patternTimer--;
    phaseTimer++;
}

void Boss::firePattern(Game* game) {
    if (!entered || phase == BossPhase::DEFEATED || !game->player) return;
    if (patternTimer > 0) return;

    float cx = centerX();
    float cy = y + height;

    switch (phase) {
    case BossPhase::PHASE_1:
        // 每隔60帧发射3颗瞄准子弹
        patternTimer = 60;
        {
            float px = game->player->centerX();
            float py = game->player->centerY();
            float dx = px - cx;
            float dy = py - cy;
            float len = sqrtf(dx * dx + dy * dy);
            if (len > 0) {
                float baseAngle = dx / len;
                game->enemyBullets.push_back(new EnemyBullet(cx - BULLET_ENEMY_WIDTH / 2.0f, cy, baseAngle * 0.5f));
                game->enemyBullets.push_back(new EnemyBullet(cx - BULLET_ENEMY_WIDTH / 2.0f - 8, cy, (baseAngle - 0.15f) * 0.5f));
                game->enemyBullets.push_back(new EnemyBullet(cx - BULLET_ENEMY_WIDTH / 2.0f + 8, cy, (baseAngle + 0.15f) * 0.5f));
            }
        }
        break;

    case BossPhase::PHASE_2:
        // 扇形弹幕：5发散射 + 每45帧一次
        patternTimer = 45;
        for (int i = -2; i <= 2; i++) {
            float angle = (float)i * 0.2f;
            game->enemyBullets.push_back(new EnemyBullet(cx - BULLET_ENEMY_WIDTH / 2.0f + i * 10.0f, cy, angle * 0.5f));
        }
        // 偶尔追加瞄准弹
        if (rand() % 3 == 0) {
            float dx = game->player->centerX() - cx;
            float len = sqrtf(dx * dx + 1.0f);
            game->enemyBullets.push_back(new EnemyBullet(cx - BULLET_ENEMY_WIDTH / 2.0f, cy, (dx / len) * 0.5f));
        }
        break;

    case BossPhase::PHASE_3:
        // 环形弹幕 + 密集射击
        patternTimer = 35;
        patternIndex = (patternIndex + 1) % 3;
        switch (patternIndex) {
        case 0:
            // 8方向环形弹幕
            for (int i = 0; i < 8; i++) {
                float angle = (float)i * 3.14159f * 2.0f / 8.0f;
                float ax = cosf(angle) * 0.6f;
                game->enemyBullets.push_back(new EnemyBullet(cx - BULLET_ENEMY_WIDTH / 2.0f, cy, ax));
            }
            break;
        case 1:
            // 扇形9发散射
            for (int i = -4; i <= 4; i++) {
                float angle = (float)i * 0.15f;
                game->enemyBullets.push_back(new EnemyBullet(cx - BULLET_ENEMY_WIDTH / 2.0f + i * 8.0f, cy, angle * 0.5f));
            }
            break;
        case 2:
            // 双向螺旋弹幕
            for (int i = 0; i < 6; i++) {
                float offset = sinf((float)(animTimer + i * 10) * 0.15f) * 0.5f;
                game->enemyBullets.push_back(new EnemyBullet(cx - BULLET_ENEMY_WIDTH / 2.0f - 15 + i * 5.0f, cy, offset));
            }
            break;
        }
        break;

    default: break;
    }
}

void Boss::render(HDC hdc) const {
    float cx = centerX();
    float cy = centerY();

    // 阶段颜色变化
    COLORREF bodyColor;
    switch (phase) {
    case BossPhase::PHASE_1: bodyColor = RGB(200, 30, 30); break;
    case BossPhase::PHASE_2: bodyColor = RGB(220, 80, 20); break;
    case BossPhase::PHASE_3: bodyColor = RGB(255, 30, 30); break;
    default: bodyColor = RGB(200, 30, 30); break;
    }

    COLORREF darkColor = RGB(
        (int)(GetRValue(bodyColor) * 0.6),
        (int)(GetGValue(bodyColor) * 0.6),
        (int)(GetBValue(bodyColor) * 0.6)
    );

    // ---- 引擎光效 ----
    if (entered) {
        for (int i = 0; i < 3; i++) {
            int flicker = 4 + rand() % 8;
            HPEN glowPen = CreatePen(PS_SOLID, 2, RGB(255, 150 + rand() % 106, 0));
            HPEN oldPen = (HPEN)SelectObject(hdc, glowPen);
            MoveToEx(hdc, (int)(cx - 15 + i * 15), (int)(y + height), nullptr);
            LineTo(hdc, (int)(cx - 15 + i * 15), (int)(y + height + flicker));
            SelectObject(hdc, oldPen);
            DeleteObject(glowPen);
        }
    }

    // ---- 主体（大型多边形） ----
    HBRUSH bodyBrush = CreateSolidBrush(bodyColor);
    HPEN bodyPen = CreatePen(PS_SOLID, 2, darkColor);
    HPEN oldPen = (HPEN)SelectObject(hdc, bodyPen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, bodyBrush);

    // 主机身
    POINT body[8] = {
        { (int)(cx),                   (int)(y) },                    // 顶部尖端
        { (int)(cx - 30),              (int)(y + 15) },
        { (int)(cx - 35),              (int)(y + 25) },
        { (int)(cx - 25),              (int)(y + 55) },
        { (int)(cx + 25),              (int)(y + 55) },
        { (int)(cx + 35),              (int)(y + 25) },
        { (int)(cx + 30),              (int)(y + 15) },
        { (int)(cx),                   (int)(y) },
    };
    Polygon(hdc, body, 8);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(bodyBrush);
    DeleteObject(bodyPen);

    // ---- 机翼 ----
    HBRUSH wingBrush = CreateSolidBrush(darkColor);
    HPEN wingPen = CreatePen(PS_SOLID, 2, darkColor);
    oldPen = (HPEN)SelectObject(hdc, wingPen);
    oldBrush = (HBRUSH)SelectObject(hdc, wingBrush);

    // 左翼
    POINT lwing[5] = {
        { (int)(cx - 30), (int)(cy - 5) },
        { (int)(cx - 65), (int)(cy) },
        { (int)(cx - 60), (int)(cy + 20) },
        { (int)(cx - 35), (int)(cy + 15) },
        { (int)(cx - 30), (int)(cy - 5) },
    };
    Polygon(hdc, lwing, 5);

    // 右翼
    POINT rwing[5] = {
        { (int)(cx + 30), (int)(cy - 5) },
        { (int)(cx + 65), (int)(cy) },
        { (int)(cx + 60), (int)(cy + 20) },
        { (int)(cx + 35), (int)(cy + 15) },
        { (int)(cx + 30), (int)(cy - 5) },
    };
    Polygon(hdc, rwing, 5);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(wingBrush);
    DeleteObject(wingPen);

    // ---- 驾驶舱 ----
    HBRUSH cockpitBrush = CreateSolidBrush(RGB(0, 200, 255));
    HPEN cockpitPen = CreatePen(PS_SOLID, 1, RGB(0, 150, 200));
    oldPen = (HPEN)SelectObject(hdc, cockpitPen);
    oldBrush = (HBRUSH)SelectObject(hdc, cockpitBrush);
    Ellipse(hdc, (int)(cx - 10), (int)(y + 5), (int)(cx + 10), (int)(y + 22));
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(cockpitBrush);
    DeleteObject(cockpitPen);

    // ---- 护甲板 ----
    if (phase == BossPhase::PHASE_2 || phase == BossPhase::PHASE_3) {
        HPEN armorPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 100));
        oldPen = (HPEN)SelectObject(hdc, armorPen);
        // 闪烁护甲线
        if (animTimer % 20 < 10) {
            MoveToEx(hdc, (int)(cx - 25), (int)(y + 20), nullptr);
            LineTo(hdc, (int)(cx + 25), (int)(y + 20));
        }
        SelectObject(hdc, oldPen);
        DeleteObject(armorPen);
    }

    // ---- 第三阶段愤怒光晕 ----
    if (phase == BossPhase::PHASE_3) {
        HPEN ragePen = CreatePen(PS_SOLID, 2, RGB(255, 50, 0));
        oldPen = (HPEN)SelectObject(hdc, ragePen);
        HBRUSH nullBr = (HBRUSH)GetStockObject(NULL_BRUSH);
        oldBrush = (HBRUSH)SelectObject(hdc, nullBr);
        int radius = 45 + (int)(sinf((float)animTimer * 0.2f) * 8.0f);
        Ellipse(hdc, (int)(cx - radius), (int)(cy - radius),
            (int)(cx + radius), (int)(cy + radius));
        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(ragePen);
    }
}

void Boss::renderHPBar(HDC hdc) const {
    if (!entered) return;

    int barWidth = 200;
    int barHeight = 12;
    int barX = WINDOW_WIDTH / 2 - barWidth / 2;
    int barY = 5;

    float hpPercent = (float)hp / maxHp;

    // 背景
    HBRUSH bgBrush = CreateSolidBrush(RGB(60, 60, 60));
    RECT bgRc = { barX, barY, barX + barWidth, barY + barHeight };
    FillRect(hdc, &bgRc, bgBrush);
    DeleteObject(bgBrush);

    // HP 颜色：绿→黄→红
    COLORREF hpColor;
    if (hpPercent > 0.5f)
        hpColor = RGB((int)(255 * (1.0f - hpPercent) * 2), 255, 0);
    else
        hpColor = RGB(255, (int)(255 * hpPercent * 2), 0);

    HBRUSH hpBrush = CreateSolidBrush(hpColor);
    RECT hpRc = { barX, barY, barX + (int)(barWidth * hpPercent), barY + barHeight };
    FillRect(hdc, &hpRc, hpBrush);
    DeleteObject(hpBrush);

    // 边框
    HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(150, 150, 150));
    HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
    HBRUSH nullBr = (HBRUSH)GetStockObject(NULL_BRUSH);
    HBRUSH oldBr = (HBRUSH)SelectObject(hdc, nullBr);
    Rectangle(hdc, barX, barY, barX + barWidth, barY + barHeight);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBr);
    DeleteObject(borderPen);

    // Boss 名称 + HP 文字
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));
    char buf[64];
    sprintf_s(buf, "BOSS  HP: %d/%d", hp, maxHp);
    RECT textRc = { barX, barY, barX + barWidth, barY + barHeight };
    DrawTextA(hdc, buf, -1, &textRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}
