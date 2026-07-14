#pragma once
#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include "Config.h"
#include "GameObject.h"

class Player;
class Enemy;
class Bullet;
class Item;

// ============================================================
// 星空粒子
// ============================================================
struct Star {
    float x, y, speed;
    int brightness, size;
    Gdiplus::Color color;
    int twinkleTimer, twinklePhase;
};

// ============================================================
// 爆炸粒子
// ============================================================
struct Particle {
    float x, y, vx, vy;
    int lifetime, maxLifetime;
    Gdiplus::Color color;
    Gdiplus::Color trailColor;
    float size;
    bool alive;
};

// ============================================================
// 游戏状态
// ============================================================
enum class GameState {
    MENU, SETTINGS, PLAYING, PAUSED,
    LEVEL_TRANSITION, GAME_OVER, VICTORY,
    LEADERBOARD, NAME_ENTRY, HELP
};

enum class MenuOption { START_GAME, LEADERBOARD, SETTINGS_MENU, EXIT, COUNT };

// ============================================================
// 文件：Game.h
// 功能：游戏主控 — GDI+ 渲染 / QPC 循环 / 素材管理
// ============================================================

class Game {
public:
    Game();
    ~Game();

    bool init(HINSTANCE hInstance);
    void run();
    void shutdown();
    void resetGame();

private:
    // ── 窗口 ──
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    bool createWindow(HINSTANCE);

    // ── 循环 ──
    void handleInput();
    void update();
    void collide();
    void render();

    // ── 子逻辑 ──
    void spawnEnemies();
    void outOfBoundsCleanup();
    void spawnParticles(float cx, float cy, int count,
                        Gdiplus::Color color, Gdiplus::Color trail,
                        float minSpd, float maxSpd, float minSz, float maxSz);
    void activateBomb();

    // ── 碰撞 ──
    void checkBulletEnemyCollisions();
    void checkEnemyBulletPlayerCollisions();
    void checkEnemyPlayerCollisions();
    void checkItemPlayerCollisions();

    // ── 渲染 ──
    void renderBackground(Gdiplus::Graphics& g);
    void renderParticles(Gdiplus::Graphics& g);
    void renderItems(Gdiplus::Graphics& g);
    void renderEnemies(Gdiplus::Graphics& g);
    void renderBullets(Gdiplus::Graphics& g);
    void renderHUD(Gdiplus::Graphics& g);
    void renderMenu(Gdiplus::Graphics& g);
    void renderSettings(Gdiplus::Graphics& g);
    void renderPauseOverlay(Gdiplus::Graphics& g);
    void renderGameOver(Gdiplus::Graphics& g);
    void renderLeaderboard(Gdiplus::Graphics& g);
    void renderNameEntry(Gdiplus::Graphics& g);
    void renderLevelTransition(Gdiplus::Graphics& g);
    void renderHelp(Gdiplus::Graphics& g);

    // ── 辅助 ──
    void renderTiledBG(Gdiplus::Graphics& g, Gdiplus::Image* img);
    void drawGlow(Gdiplus::Graphics& g, float cx, float cy, float r,
                  Gdiplus::Color c, int layers);
    void applyResolution(int idx);
    void resetStars();

    // ── 关卡 ──
    void advanceLevel();
    void startNextLevel();

    // ── 排行榜 ──
    void loadLeaderboard();
    void saveLeaderboard();
    void addScoreEntry(const char* name);
    bool isHighScore() const;

public:
    // ── 公共状态 ──
    HWND     hwnd;
    HDC      frontDC;
    Gdiplus::Image* bgImage;      // 背景素材
    Gdiplus::Image* uiFrame;      // UI 边框素材
    int   bgAlpha;                // 背景亮度 0-255（可调）
    bool  fullscreen;             // 全屏模式

    // 输入
    bool keyLeft, keyRight, keyUp, keyDown;
    bool keySpace, keyBomb, keyPause, keyShift, keyCtrl, keyHelp;
    bool spacePressed;            // 保留用于特殊操作
    int  frameCount;

    // 状态机
    GameState  state;
    MenuOption menuSelection;
    int  settingsSelection, currentResolution, stateTimer;
    int  shakeTimer, shakeIntensity;

    // 关卡
    int   currentLevel, scoreToNextLevel;
    float spawnInterval, fastEnemyChance, shootingEnemyChance;
    int   enemySpawnTimer, enemiesKilledThisLevel;
    bool  bossSpawned, bossDefeated;

    // 分数
    int score, highScore;

    // 对象容器
    Player* player;
    std::vector<Enemy*>    enemies;
    std::vector<Bullet*>   playerBullets;
    std::vector<Bullet*>   enemyBullets;
    std::vector<Item*>     items;
    std::vector<Particle>  particles;
    std::vector<Star>      stars;

    // 排行榜
    struct ScoreEntry { char name[NAME_MAX_LEN]; int score, level; char date[11]; };
    std::vector<ScoreEntry> leaderboard;
    char nameBuffer[NAME_MAX_LEN];
    int  nameLength;

private:
    // ── QPC ──
    LARGE_INTEGER qpcFreq, qpcPrev;
    double deltaTime;
    int    menuCooldown;
};
