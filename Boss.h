#pragma once
#include <windows.h>
#include <gdiplus.h>
#include "Enemy.h"

class Game;

enum class BossPhase { ENTRY, PHASE_1, PHASE_2, PHASE_3, DEFEATED };

// ============================================================
// 文件：Boss.h
// 功能：Boss — 多阶段 AI + GDI+ 渲染 + HP 条
// ============================================================

class Boss : public Enemy {
public:
    Boss(float startX, int level);

    void update() override;
    void render(Renderer& r) const override;

    BossPhase getCurrentPhase() const;
    void updatePhase();
    void firePattern(Game* game);
    void renderHPBar(Renderer& r) const;

private:
    BossPhase phase;
    int phaseTimer, patternTimer, patternIndex;
    float targetX;
    int entryTimer;
    bool entered;
    int animTimer;
};
