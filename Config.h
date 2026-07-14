#pragma once
#include <windows.h>
#include <cstdio>
#include <cwchar>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

// ============================================================
// Config.h — 全局常量 / 调色板 / 多分辨率 / 自动射击
// 所有数值基于 60 FPS 逻辑帧
// ============================================================

// ==================== 全局缩放 ====================
// GAME_SCALE = 当前分辨率高度 / 480.0f (在 Game.cpp 中定义)
extern float GAME_SCALE;

// ==================== 统一调色板 ====================
namespace Palette {
    const Gdiplus::Color DeepSpace   (255,   2,   2,  18);
    const Gdiplus::Color MenuTitle   (255, 255, 220,   0);
    const Gdiplus::Color MenuSel     (255, 255, 255, 140);
    const Gdiplus::Color MenuNormal  (255, 185, 185, 185);
    const Gdiplus::Color MenuHint    (255, 110, 110, 110);

    const Gdiplus::Color PlayerBody    (255,   0, 190, 255);
    const Gdiplus::Color PlayerDark    (255,   0, 120, 200);
    const Gdiplus::Color PlayerWing    (255, 190, 210, 230);
    const Gdiplus::Color PlayerCockpit (255,   0, 248, 255);
    const Gdiplus::Color PlayerEngine  (255, 255, 165,  20);
    const Gdiplus::Color PlayerGlow    (255,   0, 140, 240);
    const Gdiplus::Color PlayerFastGlow(255,   0, 255, 140);
    const Gdiplus::Color PlayerSlowGlow(255, 100, 180, 255);

    const Gdiplus::Color EnemyNormal     (255, 225,  38,  38);
    const Gdiplus::Color EnemyNormalDark (255, 160,  18,  18);
    const Gdiplus::Color EnemyFast       (255, 255, 165,   0);
    const Gdiplus::Color EnemyFastDark   (255, 200, 120,   0);
    const Gdiplus::Color EnemyShooting   (255, 165,  38, 225);
    const Gdiplus::Color EnemyShootingLit(255, 225, 105, 255);

    const Gdiplus::Color BossPhase1     (255, 185, 22,  22);
    const Gdiplus::Color BossPhase1Dark (255, 110, 10,  10);
    const Gdiplus::Color BossPhase2     (255, 235, 85,  15);
    const Gdiplus::Color BossPhase2Dark (255, 150, 50,   8);
    const Gdiplus::Color BossPhase3     (255, 255, 20,  20);
    const Gdiplus::Color BossPhase3Dark (255, 170, 10,  10);
    const Gdiplus::Color BossCockpit    (255,   0, 210, 255);

    const Gdiplus::Color BulletPlayer    (255, 255, 255,  60);
    const Gdiplus::Color BulletPlayerCore(255, 255, 255, 210);
    const Gdiplus::Color BulletEnemy     (255, 255,  45,  45);
    const Gdiplus::Color BulletEnemyCore (255, 255, 160, 160);

    const Gdiplus::Color ItemFirepower(255, 255, 205, 0);
    const Gdiplus::Color ItemHealth   (255, 255,  55, 55);
    const Gdiplus::Color ItemBomb     (255, 255, 125,  0);
    const Gdiplus::Color ItemShield   (255,   0, 155, 255);

    const Gdiplus::Color ParticleExplosion(255, 255, 105, 25);
    const Gdiplus::Color ParticleBomb     (255, 255, 255, 210);
}

// ==================== 窗口 ====================
extern int WINDOW_WIDTH;
extern int WINDOW_HEIGHT;
static constexpr int FRAME_INTERVAL   = 16;
static constexpr int LOGIC_HZ         = 60;
static constexpr double TARGET_FRAME_TIME = 1.0 / 60.0;

struct Resolution { int width, height; const char* label; bool fullscreen; };
static const Resolution RESOLUTIONS[] = {
    {  640,  480, "640x480    (VGA)",       false },
    {  800,  600, "800x600    (SVGA)",      false },
    { 1024,  768, "1024x768   (XGA)",       false },
    { 1280,  720, "1280x720   (HD)",        false },
    { 1920, 1080, "1920x1080  (Full HD)",   true  },
    { 2560, 1440, "2560x1440  (2K QHD)",    true  },
};
static constexpr int RESOLUTION_COUNT = 6;

// ==================== 背景 ====================
extern int BG_ALPHA;  // 0=全黑 255=原始 (默认75)

// ==================== 玩家 (60FPS) ====================
static constexpr int   PLAYER_WIDTH           = 40;
static constexpr int   PLAYER_HEIGHT          = 40;
static constexpr float PLAYER_BASE_SPEED      = 6.5f;
static constexpr float PLAYER_FAST_MULT       = 1.65f;
static constexpr float PLAYER_SLOW_MULT       = 0.38f;
static constexpr int   PLAYER_INIT_LIVES      = 3;
static constexpr int   PLAYER_MAX_LIVES       = 5;
static constexpr int   PLAYER_MAX_BOMBS       = 3;
static constexpr int   PLAYER_MAX_FIREPOWER   = 5;
static constexpr int   PLAYER_SHOOT_COOLDOWN  = 10;   // 基础射击间隔（帧）— 较慢
static constexpr int   PLAYER_FAST_SHOOT      = 4;    // 射速提升后间隔
static constexpr int   FIRERATE_DURATION      = 480;  // 射速提升持续 8 秒
static constexpr int   PLAYER_INVINCIBLE_FRAMES = 180;
static constexpr int   SHIELD_DURATION        = 600;
static constexpr int   FIREPOWER_DURATION     = 600;

// ==================== 敌机 ====================
static constexpr int   ENEMY_NORMAL_WIDTH    = 35, ENEMY_NORMAL_HEIGHT   = 35;
static constexpr int   ENEMY_FAST_WIDTH      = 25, ENEMY_FAST_HEIGHT     = 25;
static constexpr int   ENEMY_SHOOTING_WIDTH  = 35, ENEMY_SHOOTING_HEIGHT = 35;
static constexpr float ENEMY_NORMAL_SPEED   = 1.5f;
static constexpr float ENEMY_FAST_SPEED     = 3.5f;
static constexpr float ENEMY_SHOOTING_SPEED = 1.0f;
static constexpr int   ENEMY_NORMAL_SCORE   = 10;
static constexpr int   ENEMY_FAST_SCORE     = 20;
static constexpr int   ENEMY_SHOOTING_SCORE = 30;
static constexpr int   ENEMY_MAX_COUNT      = 30;

// ==================== Boss ====================
static constexpr int   BOSS_WIDTH = 80, BOSS_HEIGHT = 60;
static constexpr float BOSS_SPEED   = 0.8f;
static constexpr int   BOSS_BASE_HP = 30;
static constexpr int   BOSS_SCORE   = 5000;

// ==================== 子弹 ====================
static constexpr int   BULLET_PLAYER_WIDTH  = 4,  BULLET_PLAYER_HEIGHT = 12;
static constexpr int   BULLET_ENEMY_WIDTH   = 6,  BULLET_ENEMY_HEIGHT  = 6;
static constexpr float BULLET_PLAYER_SPEED = 8.0f;
static constexpr float BULLET_ENEMY_SPEED  = 3.0f;

// ==================== 道具 ====================
static constexpr int   ITEM_SIZE       = 20;
static constexpr float ITEM_FALL_SPEED = 1.5f;
static constexpr float ITEM_DROP_CHANCE = 0.15f;

// ==================== 粒子 ====================
static constexpr int   PARTICLE_MAX         = 800;
static constexpr int   EXPLOSION_PARTICLES  = 35;
static constexpr int   EXPLOSION_LIFE       = 30;
static constexpr float PARTICLE_GRAVITY     = 0.06f;
static constexpr float PARTICLE_DRAG        = 0.97f;
static constexpr int   BOSS_EXPLOSION_WAVES = 3;

// ==================== 关卡 ====================
static constexpr int LEVEL_COUNT             = 5;
static constexpr int LEVEL_TRANSITION_FRAMES = 120;

// ==================== 排行榜 ====================
static constexpr int LEADERBOARD_MAX = 10;
static constexpr int NAME_MAX_LEN    = 16;

// ==================== 通用兼容宏 ====================
#ifndef _countof
#define _countof(arr) (sizeof(arr)/sizeof((arr)[0]))
#endif

#ifdef __MINGW32__
#ifndef sprintf_s
#define sprintf_s(buf, sz, fmt, ...) _snprintf(buf, sz, fmt, ##__VA_ARGS__)
#endif
#ifndef strcat_s
#define strcat_s(dst, sz, src) strncat(dst, src, sz - strlen(dst) - 1)
#endif
#ifndef strncpy_s
#define strncpy_s(dst, sz, src, n) do { strncpy(dst, src, (n)<(sz)?(n):(sz)-1); dst[(sz)-1]='\0'; } while(0)
#endif
#ifndef swprintf_s
#define swprintf_s(buf, sz, fmt, ...) _snwprintf(buf, sz, fmt, ##__VA_ARGS__)
#endif
#ifndef SetProcessDPIAware
#define SetProcessDPIAware() ((void)0)
#endif
#endif
