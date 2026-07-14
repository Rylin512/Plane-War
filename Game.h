#pragma once
#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include "Config.h"
#include "GameObject.h"
#include "Renderer.h"

class Player;
class Enemy;
class Bullet;
class PlayerBullet;
class EnemyBullet;
class Item;

// ============================================================
// 星空粒子
// ============================================================
struct Star {
    float x, y, speed;
    int brightness, size;
    Gdiplus::Color color;
    int twinkleTimer, twinklePhase;
    int colorIndex; // 0-3, 对应预创建画刷索引
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
    void renderBackground(Renderer& r);
    void renderParticles(Renderer& r);
    void renderItems(Renderer& r);
    void renderEnemies(Renderer& r);
    void renderBullets(Renderer& r);
    void renderHUD(Renderer& r);
    void renderMenu(Renderer& r);
    void renderSettings(Renderer& r);
    void renderPauseOverlay(Renderer& r);
    void renderGameOver(Renderer& r);
    void renderLeaderboard(Renderer& r);
    void renderNameEntry(Renderer& r);
    void renderLevelTransition(Renderer& r);
    void renderHelp(Renderer& r);

    // ── 辅助 ──
    void renderTiledBG(Renderer& r, Gdiplus::Image* img);
    void drawGlow(Renderer& r, float cx, float cy, float radius,
                  Gdiplus::Color c, int layers);
    void applyResolution(int idx);
    void resetStars();

    // ── 持久化渲染资源 ──
    void createRenderResources();
    void destroyRenderResources();
    void recreateFonts();
    void recreateRenderer();   // 切换渲染后端时调用
    void initDirect2D();       // 尝试初始化 D2D 后端

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

    // 渲染后端
    Renderer* renderer;           // 当前活跃的渲染器 (GDI+ 或 D2D)
    bool  useDirect2D;            // true=Direct2D, false=GDI+
    bool  d2dAvailable;           // Direct2D 是否可用

    // 输入
    bool keyLeft, keyRight, keyUp, keyDown;
    bool keySpace, keyBomb, keyPause, keyShift, keyCtrl, keyHelp;
    bool spacePressed;            // 保留用于特殊操作
    int  frameCount;

    // 状态机
    GameState  state;
    MenuOption menuSelection;
    int  settingsSelection, currentResolution, currentRefreshRate, stateTimer;
    int  settingsPage;        // 0=分辨率, 1=刷新率, 2=渲染选项
    bool antiAlias;           // 反锯齿开关
    bool perfMode;            // 性能模式
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

    // ── 子弹对象池 (public: Boss 需要访问) ──
    PlayerBullet* acquirePlayerBullet(float x, float y);
    EnemyBullet*  acquireEnemyBullet(float x, float y, float angleX = 0);
    void releaseBullet(Bullet* b);

private:
    // ── QPC ──
    LARGE_INTEGER qpcFreq, qpcPrev;
    double deltaTime;
    int    menuCooldown;

    // ── 持久化渲染资源 (避免每帧分配) ──
    // 后台缓冲
    HDC                  memDC;
    HBITMAP              memBmp;
    HBITMAP              oldBmp;
    Gdiplus::Graphics*   backGraphics;
    // 缓存背景平铺
    Gdiplus::Bitmap*     cachedBackground;
    // 字体
    Gdiplus::FontFamily* ffConsolas;
    Gdiplus::FontFamily* ffArial;
    Gdiplus::Font*       fntHUD;
    Gdiplus::Font*       fntHUDSmall;
    // 常用画刷 (HUD/菜单共用)
    Gdiplus::SolidBrush* brWhite;
    Gdiplus::SolidBrush* brRed;
    Gdiplus::SolidBrush* brOrange;
    Gdiplus::SolidBrush* brYellow;
    Gdiplus::SolidBrush* brCyan;
    Gdiplus::SolidBrush* brGray;
    Gdiplus::SolidBrush* brDim;
    Gdiplus::SolidBrush* brGold;
    Gdiplus::SolidBrush* brSel;
    Gdiplus::SolidBrush* brNor;
    Gdiplus::SolidBrush* brGreen;
    Gdiplus::SolidBrush* brWarning;
    Gdiplus::SolidBrush* brDarkOverlay;
    Gdiplus::SolidBrush* brParticleTrail;
    Gdiplus::SolidBrush* brParticleMain;
    // 常用画笔
    Gdiplus::Pen*        penSeparator;
    // 星空画刷 (4种预定义颜色)
    Gdiplus::SolidBrush* starBrushes[4];

    // ── 子弹对象池 ──
    std::vector<PlayerBullet*> playerBulletPool;
    std::vector<EnemyBullet*>  enemyBulletPool;
};
