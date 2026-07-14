#pragma once
#include <windows.h>
#include <cstdio>
#include <cwchar>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

// ============================================================
// Config.h — 全局常量 / 调色板 / 多分辨率 / 自动射击
// 所有数值基于 60 FPS 逻辑帧
// ============================================================

// ==================== 全局缩放 ====================
// GAME_SCALE = 当前分辨率高度 / 480.0f (在 Game.cpp 中定义)
extern float GAME_SCALE;

// ==================== 统一调色板（运行时可变，支持皮肤切换） ====================
namespace Palette {
    extern Gdiplus::Color DeepSpace, MenuTitle, MenuSel, MenuNormal, MenuHint;
    extern Gdiplus::Color PlayerBody, PlayerDark, PlayerWing, PlayerCockpit, PlayerEngine, PlayerGlow, PlayerFastGlow, PlayerSlowGlow;
    extern Gdiplus::Color EnemyNormal, EnemyNormalDark, EnemyFast, EnemyFastDark, EnemyShooting, EnemyShootingLit;
    extern Gdiplus::Color BossPhase1, BossPhase1Dark, BossPhase2, BossPhase2Dark, BossPhase3, BossPhase3Dark, BossCockpit;
    extern Gdiplus::Color BulletPlayer, BulletPlayerCore, BulletEnemy, BulletEnemyCore;
    extern Gdiplus::Color ItemFirepower, ItemHealth, ItemBomb, ItemShield;
    extern Gdiplus::Color ParticleExplosion, ParticleBomb;
}

// 皮肤预设
struct SkinColors {
    const wchar_t* name;
    Gdiplus::Color playerBody, playerDark, playerWing, playerCockpit, playerEngine, playerGlow, playerFastGlow, playerSlowGlow;
    Gdiplus::Color enemyNormal, enemyNormalDark, enemyFast, enemyFastDark, enemyShooting, enemyShootingLit;
    Gdiplus::Color bossP1, bossP1D, bossP2, bossP2D, bossP3, bossP3D, bossCockpit;
    Gdiplus::Color bulletPlayer, bulletPlayerCore, bulletEnemy, bulletEnemyCore;
    Gdiplus::Color itemFire, itemHealth, itemBomb, itemShield;
    Gdiplus::Color particleExplosion, particleBomb;
};

extern SkinColors SKINS[];
extern int SKIN_COUNT;
void applySkin(int idx);  // 将皮肤颜色复制到 Palette

// ==================== 窗口 ====================
extern int WINDOW_WIDTH;
extern int WINDOW_HEIGHT;
static constexpr int FRAME_INTERVAL   = 16;
static constexpr int LOGIC_HZ         = 60;
extern double TARGET_FRAME_TIME;  // 渲染帧率上限（不影响游戏逻辑速度）

// 刷新率预设
static constexpr int REFRESH_RATES[]  = { 60, 75, 120, 144, 165, 240 };
static constexpr int REFRESH_RATE_COUNT = 6;
static constexpr int DEFAULT_REFRESH_RATE = 0;  // 60Hz

// 游戏速度档位（逻辑 tick 速率）
static constexpr int TICK_RATES[]     = { 45, 60, 90 };
static constexpr int TICK_RATE_COUNT  = 3;
static constexpr int DEFAULT_TICK     = 1;  // 60 tick/s (中速)

// 宽高比分类
enum AspectRatio { AR_4_3, AR_16_9, AR_16_10, AR_21_9, AR_32_9, AR_COUNT };

struct Resolution {
    int width, height;
    const char* label;
    bool fullscreen;
    AspectRatio aspect;
};

static const Resolution RESOLUTIONS[] = {
    // 4:3
    {   640,  480,  "640x480    (VGA)",        false, AR_4_3  },
    {   800,  600,  "800x600    (SVGA)",       false, AR_4_3  },
    {  1024,  768,  "1024x768   (XGA)",        false, AR_4_3  },
    {  1280,  960,  "1280x960   (SXGA)",       false, AR_4_3  },
    {  1400, 1050,  "1400x1050  (SXGA+)",      false, AR_4_3  },
    // 16:9
    {  1280,  720,  "1280x720   (HD)",         false, AR_16_9 },
    {  1366,  768,  "1366x768   (WXGA)",       false, AR_16_9 },
    {  1600,  900,  "1600x900   (HD+)",        false, AR_16_9 },
    {  1920, 1080,  "1920x1080  (Full HD)",    true,  AR_16_9 },
    {  2560, 1440,  "2560x1440  (2K QHD)",     true,  AR_16_9 },
    {  3840, 2160,  "3840x2160  (4K UHD)",     true,  AR_16_9 },
    // 16:10
    {  1280,  800,  "1280x800   (WXGA)",       false, AR_16_10 },
    {  1440,  900,  "1440x900   (WXGA+)",      false, AR_16_10 },
    {  1680, 1050,  "1680x1050  (WSXGA+)",     false, AR_16_10 },
    {  1920, 1200,  "1920x1200  (WUXGA)",      false, AR_16_10 },
    {  2560, 1600,  "2560x1600  (WQXGA)",      true,  AR_16_10 },
    // 21:9
    {  2560, 1080,  "2560x1080  (UWHD)",       true,  AR_21_9  },
    {  3440, 1440,  "3440x1440  (UWQHD)",      true,  AR_21_9  },
    // 32:9
    {  3840, 1080,  "3840x1080  (DFHD)",       true,  AR_32_9  },
    {  5120, 1440,  "5120x1440  (DQHD)",       true,  AR_32_9  },
};
static constexpr int RESOLUTION_COUNT = 20;

// 宽高比标签
static const char* ASPECT_LABELS[] = { "4:3", "16:9", "16:10", "21:9", "32:9" };

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
