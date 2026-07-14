#pragma once
#include <windows.h>
#include "Enemy.h"

class Game;

// ============================================================
// 文件：Boss.h
// 功能：Boss 敌机 — 血条、多阶段攻击模式
// ============================================================

// Boss 攻击阶段
enum class BossPhase {
    ENTRY,      // 入场动画
    PHASE_1,    // HP 100%-66%: 缓慢移动 + 瞄准射击
    PHASE_2,    // HP 66%-33%: 加速移动 + 扇形弹幕
    PHASE_3,    // HP 33%-0%:  激进 + 环形弹幕
    DEFEATED    // 被击毁
};

class Boss : public Enemy {
public:
    Boss(float startX, int level);

    void update() override;
    void render(HDC hdc) const override;

    // Boss 特有方法
    BossPhase getCurrentPhase() const;
    void updatePhase();
    void firePattern(Game* game);

    // 渲染血条
    void renderHPBar(HDC hdc) const;

private:
    BossPhase phase;
    int phaseTimer;         // 当前阶段计时器
    int patternTimer;       // 弹幕模式计时器
    int patternIndex;       // 当前弹幕模式索引
    float targetX;          // 移动目标X
    int entryTimer;         // 入场计时
    bool entered;           // 是否已完成入场

    // Boss 外观动画
    int animTimer;
    int wingAngle;          // 翅膀动画角度
};
