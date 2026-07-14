#pragma once
#include <windows.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include "Config.h"
#include "GameObject.h"

// 前向声明
class Player;
class Enemy;
class Bullet;
class Item;

// 星空背景粒子
struct Star {
    float x, y;
    float speed;
    int brightness;
};

// 爆炸特效粒子
struct Particle {
    float x, y;
    float vx, vy;
    int lifetime;
    int maxLifetime;
    COLORREF color;
    bool alive;
};

// ============================================================
// 文件：Game.h
// 功能：游戏主控类 — 窗口管理、游戏循环、状态机、碰撞调度
// ============================================================

// 游戏状态枚举
enum class GameState {
    MENU,
    SETTINGS,
    PLAYING,
    PAUSED,
    LEVEL_TRANSITION,
    GAME_OVER,
    VICTORY,
    LEADERBOARD,
    NAME_ENTRY
};

// 菜单选项
enum class MenuOption {
    START_GAME,
    LEADERBOARD,
    SETTINGS_MENU,
    EXIT,
    COUNT
};

class Game {
public:
    Game();
    ~Game();

    // 初始化和运行
    bool init(HINSTANCE hInstance);
    void run();
    void shutdown();

    // 游戏重置
    void resetGame();

private:
    // Windows 窗口相关
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    bool createWindow(HINSTANCE hInstance);
    void processMessages();

    // 游戏循环各阶段
    void handleInput();
    void update();
    void collide();
    void render();

    // 子阶段
    void spawnEnemies();
    void outOfBoundsCleanup();
    void spawnParticles(float cx, float cy, int count, COLORREF color);
    void activateBomb();

    // 碰撞检测
    void checkBulletEnemyCollisions();
    void checkEnemyBulletPlayerCollisions();
    void checkEnemyPlayerCollisions();
    void checkItemPlayerCollisions();

    // 渲染子函数
    void renderBackground();
    void renderPlayer();
    void renderEnemies();
    void renderBullets();
    void renderItems();
    void renderParticles();
    void renderHUD();
    void renderMenu();
    void renderSettings();
    void renderPauseMenu();
    void renderGameOver();
    void renderLeaderboard();
    void renderNameEntry();
    void renderLevelTransition();
    void renderBossHPBar();

    // 分辨率
    void applyResolution(int index);
    void resetStars();

    // 关卡管理
    void advanceLevel();
    void startNextLevel();

    // 排行榜
    void loadLeaderboard();
    void saveLeaderboard();
    void addScoreEntry(const char* name);
    bool isHighScore() const;

public:
    // 游戏状态（供其他模块访问）
    HWND hwnd;
    HDC backDC;             // 后台缓冲 DC
    HBITMAP backBitmap;     // 后台缓冲位图
    HDC frontDC;            // 窗口 DC

    // 输入状态
    bool keyLeft, keyRight, keyUp, keyDown;
    bool keySpace, keyBomb, keyPause;
    bool spacePressed;      // 空格单次触发

    // 帧计数器
    int frameCount;

    // 游戏状态
    GameState state;
    MenuOption menuSelection;
    int settingsSelection;  // 设置菜单中的分辨率选择
    int currentResolution;  // 当前分辨率索引
    int stateTimer;         // 状态计时器（用于过场动画等）

    // 屏幕震动
    int shakeTimer;
    int shakeIntensity;

    // 关卡
    int currentLevel;
    int scoreToNextLevel;
    float spawnInterval;
    float fastEnemyChance;
    float shootingEnemyChance;
    int enemySpawnTimer;
    int enemiesKilledThisLevel;
    bool bossSpawned;
    bool bossDefeated;

    // 分数与生命
    int score;
    int highScore;

    // 对象容器
    Player* player;
    std::vector<Enemy*> enemies;
    std::vector<Bullet*> playerBullets;
    std::vector<Bullet*> enemyBullets;
    std::vector<Item*> items;
    std::vector<Particle> particles;
    std::vector<Star> stars;

    // 排行榜
    struct ScoreEntry {
        char name[NAME_MAX_LEN];
        int score;
        int level;
        char date[11];
    };
    std::vector<ScoreEntry> leaderboard;
    char nameBuffer[NAME_MAX_LEN];
    int nameLength;
};
