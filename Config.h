#pragma once

// ============================================================
// 文件：Config.h
// 功能：游戏全局常量配置
// ============================================================

// ---------- 窗口设置 ----------
inline int WINDOW_WIDTH = 640;     // 可运行时更改
inline int WINDOW_HEIGHT = 480;
constexpr int FRAME_INTERVAL = 33;  // 30 FPS ≈ 33ms 每帧

// 预设分辨率
struct Resolution {
    int width, height;
    const char* label;
};
inline const Resolution RESOLUTIONS[] = {
    { 640,  480,  "640 x 480   (VGA)" },
    { 800,  600,  "800 x 600   (SVGA)" },
    { 1024, 768,  "1024 x 768  (XGA)" },
    { 1280, 720,  "1280 x 720  (HD)" },
};
constexpr int RESOLUTION_COUNT = 4;

// ---------- 玩家设置 ----------
constexpr int PLAYER_WIDTH = 40;
constexpr int PLAYER_HEIGHT = 40;
constexpr float PLAYER_SPEED = 5.0f;
constexpr int PLAYER_INIT_LIVES = 3;
constexpr int PLAYER_MAX_LIVES = 5;
constexpr int PLAYER_INVINCIBLE_FRAMES = 90;  // 无敌时间（帧数）
constexpr int PLAYER_SHOOT_COOLDOWN = 8;      // 射击冷却（帧数）
constexpr int PLAYER_MAX_FIREPOWER = 5;       // 最大火力等级
constexpr int PLAYER_MAX_BOMBS = 3;           // 最大炸弹数

// ---------- 敌机设置 ----------
constexpr int ENEMY_NORMAL_WIDTH = 35;
constexpr int ENEMY_NORMAL_HEIGHT = 35;
constexpr int ENEMY_FAST_WIDTH = 25;
constexpr int ENEMY_FAST_HEIGHT = 25;
constexpr int ENEMY_SHOOTING_WIDTH = 35;
constexpr int ENEMY_SHOOTING_HEIGHT = 35;

constexpr float ENEMY_NORMAL_SPEED = 1.5f;
constexpr float ENEMY_FAST_SPEED = 3.5f;
constexpr float ENEMY_SHOOTING_SPEED = 1.0f;

constexpr int ENEMY_NORMAL_SCORE = 10;
constexpr int ENEMY_FAST_SCORE = 20;
constexpr int ENEMY_SHOOTING_SCORE = 30;

constexpr int ENEMY_MAX_COUNT = 30;

// ---------- Boss 设置 ----------
constexpr int BOSS_WIDTH = 80;
constexpr int BOSS_HEIGHT = 60;
constexpr float BOSS_SPEED = 0.8f;
constexpr int BOSS_BASE_HP = 30;
constexpr int BOSS_SCORE = 5000;

// ---------- 子弹设置 ----------
constexpr int BULLET_PLAYER_WIDTH = 4;
constexpr int BULLET_PLAYER_HEIGHT = 12;
constexpr int BULLET_ENEMY_WIDTH = 6;
constexpr int BULLET_ENEMY_HEIGHT = 6;
constexpr float BULLET_PLAYER_SPEED = 8.0f;
constexpr float BULLET_ENEMY_SPEED = 3.0f;
constexpr int PLAYER_BULLET_MAX = 60;
constexpr int ENEMY_BULLET_MAX = 100;

// ---------- 道具设置 ----------
constexpr int ITEM_WIDTH = 20;
constexpr int ITEM_HEIGHT = 20;
constexpr float ITEM_FALL_SPEED = 1.5f;
constexpr float ITEM_DROP_CHANCE = 0.15f;  // 15% 掉落概率
constexpr int SHIELD_DURATION = 300;        // 护盾持续帧数（10秒）
constexpr int FIREPOWER_DURATION = 300;     // 火力增强持续帧数

// ---------- 粒子设置 ----------
constexpr int PARTICLE_MAX = 500;
constexpr int EXPLOSION_PARTICLE_COUNT = 25;
constexpr int EXPLOSION_PARTICLE_LIFE = 20;
constexpr int BOSS_EXPLOSION_WAVES = 3;

// ---------- 关卡设置 ----------
constexpr int LEVEL_COUNT = 5;
constexpr int LEVEL_TRANSITION_FRAMES = 60;  // 关卡切换显示帧数

// ---------- 排行榜设置 ----------
constexpr int LEADERBOARD_MAX = 10;
constexpr int NAME_MAX_LEN = 16;
