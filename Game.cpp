#include "Game.h"
#include "Player.h"
#include "Enemy.h"
#include "Boss.h"
#include "Bullet.h"
#include "Item.h"
#include <cstdio>
#include <cmath>

// ============================================================
// 文件：Game.cpp
// 功能：游戏主控实现 — Windows GDI、游戏循环、碰撞调度
// ============================================================

static Game* g_game = nullptr;

// ---------- 窗口过程 ----------

LRESULT CALLBACK Game::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_KEYDOWN:
        if (g_game && wParam == VK_ESCAPE) {
            if (g_game->state == GameState::PLAYING) {
                g_game->state = GameState::PAUSED;
            } else if (g_game->state == GameState::PAUSED) {
                // 返回主菜单前清理所有游戏对象
                if (g_game->player) { delete g_game->player; g_game->player = nullptr; }
                for (auto* e : g_game->enemies) delete e;
                for (auto* b : g_game->playerBullets) delete b;
                for (auto* b : g_game->enemyBullets) delete b;
                for (auto* i : g_game->items) delete i;
                g_game->enemies.clear();
                g_game->playerBullets.clear();
                g_game->enemyBullets.clear();
                g_game->items.clear();
                g_game->particles.clear();
                g_game->state = GameState::MENU;
            }
        }
        return 0;
    case WM_ACTIVATE:
        if (g_game && g_game->state == GameState::PLAYING && LOWORD(wParam) == WA_INACTIVE)
            g_game->state = GameState::PAUSED;
        return 0;
    case WM_SIZE:
        if (g_game && g_game->backDC) {
            RECT rc; GetClientRect(hwnd, &rc);
            DeleteDC(g_game->backDC);
            DeleteObject(g_game->backBitmap);
            HDC screenDC = GetDC(hwnd);
            g_game->backDC = CreateCompatibleDC(screenDC);
            g_game->backBitmap = CreateCompatibleBitmap(screenDC, rc.right, rc.bottom);
            SelectObject(g_game->backDC, g_game->backBitmap);
            ReleaseDC(hwnd, screenDC);
        }
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// ---------- 构造/析构 ----------

Game::Game()
    : hwnd(nullptr), backDC(nullptr), backBitmap(nullptr), frontDC(nullptr)
    , keyLeft(false), keyRight(false), keyUp(false), keyDown(false)
    , keySpace(false), keyBomb(false), keyPause(false), spacePressed(false)
    , frameCount(0)
    , state(GameState::MENU), menuSelection(MenuOption::START_GAME)
    , settingsSelection(0), currentResolution(0), stateTimer(0)
    , shakeTimer(0), shakeIntensity(0)
    , currentLevel(0), scoreToNextLevel(500), spawnInterval(40.0f)
    , fastEnemyChance(0.1f), shootingEnemyChance(0.0f)
    , enemySpawnTimer(0), enemiesKilledThisLevel(0)
    , bossSpawned(false), bossDefeated(false)
    , score(0), highScore(0)
    , player(nullptr), nameLength(0)
{
    g_game = this;
    memset(nameBuffer, 0, sizeof(nameBuffer));
    srand((unsigned int)time(nullptr));
}

Game::~Game() { shutdown(); }

// ---------- 窗口创建 ----------

bool Game::createWindow(HINSTANCE hInstance) {
    WNDCLASSA wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "PlaneWarWindowClass";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    if (!RegisterClassA(&wc)) return false;

    RECT rc = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX, FALSE);

    hwnd = CreateWindowExA(0, "PlaneWarWindowClass", "Plane War",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr, hInstance, nullptr);
    if (!hwnd) return false;

    frontDC = GetDC(hwnd);
    backDC = CreateCompatibleDC(frontDC);
    backBitmap = CreateCompatibleBitmap(frontDC, WINDOW_WIDTH, WINDOW_HEIGHT);
    SelectObject(backDC, backBitmap);

    HFONT defFont = CreateFontA(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Consolas");
    SelectObject(backDC, defFont);

    ShowWindow(hwnd, SW_SHOW);
    return true;
}

bool Game::init(HINSTANCE hInstance) {
    // 应用默认分辨率
    WINDOW_WIDTH = RESOLUTIONS[currentResolution].width;
    WINDOW_HEIGHT = RESOLUTIONS[currentResolution].height;

    if (!createWindow(hInstance)) return false;
    loadLeaderboard();
    resetStars();
    return true;
}

void Game::applyResolution(int index) {
    if (index < 0 || index >= RESOLUTION_COUNT) return;
    if (index == currentResolution) return;

    currentResolution = index;
    WINDOW_WIDTH = RESOLUTIONS[index].width;
    WINDOW_HEIGHT = RESOLUTIONS[index].height;

    // 重建窗口和缓冲区
    if (hwnd) {
        // 释放旧缓冲
        if (backDC) { DeleteDC(backDC); backDC = nullptr; }
        if (backBitmap) { DeleteObject(backBitmap); backBitmap = nullptr; }
        if (frontDC) { ReleaseDC(hwnd, frontDC); frontDC = nullptr; }

        // 调整窗口大小
        RECT rc = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX, FALSE);
        SetWindowPos(hwnd, nullptr, 0, 0,
            rc.right - rc.left, rc.bottom - rc.top,
            SWP_NOMOVE | SWP_NOZORDER);

        // 重建后台缓冲
        frontDC = GetDC(hwnd);
        backDC = CreateCompatibleDC(frontDC);
        backBitmap = CreateCompatibleBitmap(frontDC, WINDOW_WIDTH, WINDOW_HEIGHT);
        SelectObject(backDC, backBitmap);

        // 重设字体
        HFONT defFont = CreateFontA(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Consolas");
        SelectObject(backDC, defFont);
    }

    // 重置星空
    resetStars();

    // 调整玩家位置（如果在游戏中）
    if (player) {
        player->x = WINDOW_WIDTH / 2.0f - PLAYER_WIDTH / 2.0f;
        player->y = WINDOW_HEIGHT - 80.0f;
    }
}

void Game::resetStars() {
    stars.clear();
    int starCount = WINDOW_WIDTH * WINDOW_HEIGHT / 3800;  // 按面积比例缩放
    for (int i = 0; i < starCount; i++) {
        Star s;
        s.x = (float)(rand() % WINDOW_WIDTH);
        s.y = (float)(rand() % WINDOW_HEIGHT);
        s.speed = 0.5f + (float)(rand() % 100) / 100.0f;
        s.brightness = 100 + rand() % 156;
        stars.push_back(s);
    }
}

void Game::shutdown() {
    if (player) { delete player; player = nullptr; }
    for (auto* e : enemies) delete e;
    for (auto* b : playerBullets) delete b;
    for (auto* b : enemyBullets) delete b;
    for (auto* i : items) delete i;
    enemies.clear();
    playerBullets.clear();
    enemyBullets.clear();
    items.clear();
    particles.clear();
    if (backDC) { DeleteDC(backDC); backDC = nullptr; }
    if (backBitmap) { DeleteObject(backBitmap); backBitmap = nullptr; }
    if (frontDC && hwnd) { ReleaseDC(hwnd, frontDC); frontDC = nullptr; }
    if (hwnd) { DestroyWindow(hwnd); hwnd = nullptr; }
}

// ---------- 游戏重置 ----------

void Game::resetGame() {
    if (player) { delete player; player = nullptr; }
    for (auto* e : enemies) delete e;
    for (auto* b : playerBullets) delete b;
    for (auto* b : enemyBullets) delete b;
    for (auto* i : items) delete i;
    enemies.clear();
    playerBullets.clear();
    enemyBullets.clear();
    items.clear();
    particles.clear();

    player = new Player();
    score = 0;
    currentLevel = 1;
    scoreToNextLevel = 500;
    spawnInterval = 40.0f;
    fastEnemyChance = 0.1f;
    shootingEnemyChance = 0.0f;
    enemySpawnTimer = 0;
    enemiesKilledThisLevel = 0;
    bossSpawned = false;
    bossDefeated = false;
    shakeTimer = 0;
    shakeIntensity = 0;
    state = GameState::PLAYING;
    stateTimer = 0;
    frameCount = 0;
}

// ---------- 主循环 ----------

void Game::run() {
    MSG msg;
    while (true) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) { hwnd = nullptr; return; }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (!hwnd) break;

        DWORD frameStart = GetTickCount();
        handleInput();
        update();
        collide();
        render();

        DWORD elapsed = GetTickCount() - frameStart;
        if (elapsed < FRAME_INTERVAL) Sleep(FRAME_INTERVAL - elapsed);
        frameCount++;
    }
}

// ---------- 输入处理 ----------

void Game::handleInput() {
    keyLeft   = (GetAsyncKeyState(VK_LEFT)  & 0x8000) || (GetAsyncKeyState('A') & 0x8000);
    keyRight  = (GetAsyncKeyState(VK_RIGHT) & 0x8000) || (GetAsyncKeyState('D') & 0x8000);
    keyUp     = (GetAsyncKeyState(VK_UP)    & 0x8000) || (GetAsyncKeyState('W') & 0x8000);
    keyDown   = (GetAsyncKeyState(VK_DOWN)  & 0x8000) || (GetAsyncKeyState('S') & 0x8000);
    keySpace  = (GetAsyncKeyState(VK_SPACE) & 0x8000);
    keyBomb   = (GetAsyncKeyState('B')      & 0x8000);
    keyPause  = (GetAsyncKeyState('P')      & 0x8000);

    static bool prevSpace = false;
    spacePressed = keySpace && !prevSpace;
    prevSpace = keySpace;

    static bool prevPause = false;
    if (keyPause && !prevPause) {
        if (state == GameState::PLAYING) state = GameState::PAUSED;
        else if (state == GameState::PAUSED) state = GameState::PLAYING;
    }
    prevPause = keyPause;

    // 炸弹触发
    static bool prevBomb = false;
    if (keyBomb && !prevBomb && state == GameState::PLAYING && player && player->bombCount > 0) {
        activateBomb();
    }
    prevBomb = keyBomb;

    // ---- 菜单输入 ----
    if (state == GameState::MENU) {
        static bool prevUp = false, prevDown = false;
        bool upP = (GetAsyncKeyState(VK_UP) & 0x8000) || (GetAsyncKeyState('W') & 0x8000);
        bool dnP = (GetAsyncKeyState(VK_DOWN) & 0x8000) || (GetAsyncKeyState('S') & 0x8000);
        int menuCount = 4;
        if (upP && !prevUp) { int s = (int)menuSelection; s = (s - 1 + menuCount) % menuCount; menuSelection = (MenuOption)s; }
        if (dnP && !prevDown) { int s = (int)menuSelection; s = (s + 1) % menuCount; menuSelection = (MenuOption)s; }
        prevUp = upP; prevDown = dnP;
        if (GetAsyncKeyState(VK_RETURN) & 0x8000) {
            switch (menuSelection) {
            case MenuOption::START_GAME: resetGame(); break;
            case MenuOption::LEADERBOARD: state = GameState::LEADERBOARD; break;
            case MenuOption::SETTINGS_MENU:
                settingsSelection = currentResolution;
                state = GameState::SETTINGS;
                break;
            case MenuOption::EXIT: PostQuitMessage(0); hwnd = nullptr; break;
            }
        }
    }

    // ---- 设置菜单输入 ----
    if (state == GameState::SETTINGS) {
        static bool prevUp2 = false, prevDown2 = false, prevEnter2 = false, prevEsc2 = false;
        bool upP = (GetAsyncKeyState(VK_UP) & 0x8000) || (GetAsyncKeyState('W') & 0x8000);
        bool dnP = (GetAsyncKeyState(VK_DOWN) & 0x8000) || (GetAsyncKeyState('S') & 0x8000);
        bool enterP = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;
        bool escP = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
        if (upP && !prevUp2) { settingsSelection = (settingsSelection - 1 + RESOLUTION_COUNT) % RESOLUTION_COUNT; }
        if (dnP && !prevDown2) { settingsSelection = (settingsSelection + 1) % RESOLUTION_COUNT; }
        if (enterP && !prevEnter2) {
            applyResolution(settingsSelection);
            state = GameState::MENU;
        }
        if (escP && !prevEsc2) {
            state = GameState::MENU;
        }
        prevUp2 = upP; prevDown2 = dnP; prevEnter2 = enterP; prevEsc2 = escP;
    }

    // ---- 名字输入 ----
    if (state == GameState::NAME_ENTRY) {
        static bool prevEnter2 = false;
        for (int k = 'A'; k <= 'Z'; k++) {
            static bool prevKey[26] = {};
            bool pressed = (GetAsyncKeyState(k) & 0x8000) != 0;
            if (pressed && !prevKey[k - 'A'] && nameLength < 3) {
                nameBuffer[nameLength++] = (char)k;
            }
            prevKey[k - 'A'] = pressed;
        }
        // 退格
        static bool prevBack = false;
        bool backPressed = (GetAsyncKeyState(VK_BACK) & 0x8000) != 0;
        if (backPressed && !prevBack && nameLength > 0) {
            nameLength--;
            nameBuffer[nameLength] = '\0';
        }
        prevBack = backPressed;
        // Enter 确认
        bool enter2 = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;
        if (enter2 && !prevEnter2 && nameLength > 0) {
            nameBuffer[nameLength] = '\0';
            addScoreEntry(nameBuffer);
            state = GameState::LEADERBOARD;
        }
        prevEnter2 = enter2;
        // ESC 跳过
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            state = GameState::MENU;
        }
    }

    // ---- 游戏结束 / 胜利 ----
    if ((state == GameState::GAME_OVER || state == GameState::VICTORY) &&
        (GetAsyncKeyState(VK_RETURN) & 0x8000)) {
        if (isHighScore()) {
            nameLength = 0;
            memset(nameBuffer, 0, sizeof(nameBuffer));
            state = GameState::NAME_ENTRY;
        } else {
            state = GameState::MENU;
        }
    }
    if (state == GameState::LEADERBOARD && (GetAsyncKeyState(VK_ESCAPE) & 0x8000)) {
        state = GameState::MENU;
    }
}

// ---------- 更新逻辑 ----------

void Game::update() {
    if (state != GameState::PLAYING && state != GameState::LEVEL_TRANSITION) return;

    if (shakeTimer > 0) shakeTimer--;

    // 星空滚动
    for (auto& s : stars) {
        s.y += s.speed;
        if (s.y > WINDOW_HEIGHT) { s.y = -2; s.x = (float)(rand() % WINDOW_WIDTH); }
    }

    // 关卡过渡计时器（必须在 early return 之前）
    if (state == GameState::LEVEL_TRANSITION) {
        stateTimer--;
        if (stateTimer <= 0) {
            state = GameState::PLAYING;  // 过场结束，回到游戏
        }
    }

    if (state != GameState::PLAYING) return;

    // ---- 玩家更新 ----
    if (player) {
        float spd = PLAYER_SPEED;
        player->speedX = 0; player->speedY = 0;
        if (keyLeft)  player->speedX = -spd;
        if (keyRight) player->speedX =  spd;
        if (keyUp)    player->speedY = -spd;
        if (keyDown)  player->speedY =  spd;
        player->update();

        // 射击
        if (spacePressed && player->canShoot()) {
            player->resetShootCooldown();
            float cx = player->centerX();
            float by = player->y;
            int lv = player->firepowerLevel;

            if (lv >= 1) playerBullets.push_back(new PlayerBullet(cx - BULLET_PLAYER_WIDTH / 2.0f, by));
            if (lv >= 2) {
                playerBullets.push_back(new PlayerBullet(cx - BULLET_PLAYER_WIDTH / 2.0f - 8, by + 4));
                playerBullets.push_back(new PlayerBullet(cx - BULLET_PLAYER_WIDTH / 2.0f + 8, by + 4));
            }
            if (lv >= 3) {
                playerBullets.push_back(new PlayerBullet(cx - BULLET_PLAYER_WIDTH / 2.0f - 14, by + 8));
                playerBullets.push_back(new PlayerBullet(cx - BULLET_PLAYER_WIDTH / 2.0f + 14, by + 8));
            }
            if (lv >= 4) {
                playerBullets.push_back(new PlayerBullet(cx - BULLET_PLAYER_WIDTH / 2.0f - 4, by - 4));
                playerBullets.push_back(new PlayerBullet(cx - BULLET_PLAYER_WIDTH / 2.0f + 4, by - 4));
            }
            if (lv >= 5) {
                playerBullets.push_back(new PlayerBullet(cx - BULLET_PLAYER_WIDTH / 2.0f - 10, by - 4));
                playerBullets.push_back(new PlayerBullet(cx - BULLET_PLAYER_WIDTH / 2.0f + 10, by - 4));
            }
        }

        // 检查关卡升级（冷却结束后才检查）
        if (score >= scoreToNextLevel && !bossSpawned && state == GameState::PLAYING
            && enemySpawnTimer <= 0) {
            advanceLevel();
        }
    }

    // ---- 敌机生成 ----
    spawnEnemies();

    // ---- 敌机更新 ----
    for (auto* e : enemies) {
        e->update();
        // 射击敌机开火
        if (e->getType() == EnemyType::SHOOTING && e->shouldShoot(frameCount) && player) {
            e->resetShootTimer();
            float dx = player->centerX() - e->centerX();
            float dy = player->centerY() - e->centerY();
            float len = sqrtf(dx * dx + dy * dy);
            if (len > 0) {
                float angleX = (dx / len) * BULLET_ENEMY_SPEED * 0.4f;
                enemyBullets.push_back(new EnemyBullet(e->centerX() - 3, e->y + e->height, angleX));
            } else {
                enemyBullets.push_back(new EnemyBullet(e->centerX() - 3, e->y + e->height));
            }
        }
        // Boss 开火
        if (e->getType() == EnemyType::BOSS) {
            Boss* boss = static_cast<Boss*>(e);
            boss->firePattern(this);
        }
    }

    // ---- 子弹更新 ----
    for (auto* b : playerBullets) b->update();
    for (auto* b : enemyBullets) b->update();

    // ---- 道具更新 ----
    for (auto* it : items) it->update();

    // ---- 粒子更新 ----
    for (auto& p : particles) {
        if (!p.alive) continue;
        p.x += p.vx;
        p.y += p.vy;
        p.lifetime--;
        if (p.lifetime <= 0) p.alive = false;
    }

    // ---- 越界清理 ----
    outOfBoundsCleanup();

    // ---- Boss被击败后推进关卡 ----
    if (bossDefeated && state == GameState::PLAYING) {
        bool bossStillThere = false;
        for (auto* e : enemies) {
            if (e->getType() == EnemyType::BOSS) { bossStillThere = true; break; }
        }
        if (!bossStillThere) {
            bossDefeated = false;
            if (state != GameState::VICTORY) {
                startNextLevel();  // 进入下一关（含过场动画）
            }
        }
    }

}

void Game::spawnEnemies() {
    if (enemySpawnTimer > 0) { enemySpawnTimer--; return; }
    if ((int)enemies.size() >= ENEMY_MAX_COUNT) return;

    // Boss存活时不生成普通敌机；若无Boss但标记还在则重置
    bool bossAlive = false;
    for (auto* e : enemies) {
        if (e->getType() == EnemyType::BOSS && e->alive) { bossAlive = true; break; }
    }
    if (bossAlive) return;
    if (bossSpawned && !bossAlive) {
        bossSpawned = false;  // Boss已被炸弹等清除，恢复普通敌机生成
    }

    // 重置计时器
    enemySpawnTimer = (int)spawnInterval + rand() % 10;

    float startX = (float)(rand() % (WINDOW_WIDTH - ENEMY_NORMAL_WIDTH));

    // 随机选择敌机类型
    float roll = (float)(rand() % 100) / 100.0f;

    if (roll < shootingEnemyChance) {
        enemies.push_back(new ShootingEnemy(startX));
    } else if (roll < shootingEnemyChance + fastEnemyChance) {
        enemies.push_back(new FastEnemy(startX));
    } else {
        // 部分普通敌机使用正弦波移动
        NormalEnemy* ne = new NormalEnemy(startX);
        if (currentLevel >= 2 && (rand() % 100) < 30) {
            ne->movePattern = MovePattern::SINE;
            ne->sineAmplitude = 2.0f + (float)(rand() % 3);
            ne->sinePhase = (float)(rand() % 628) / 100.0f;
        }
        enemies.push_back(ne);
    }
}

void Game::outOfBoundsCleanup() {
    // 清理越界敌机
    for (auto it = enemies.begin(); it != enemies.end(); ) {
        if (!(*it)->alive || (*it)->y > WINDOW_HEIGHT + 50) {
            delete *it;
            it = enemies.erase(it);
        } else { ++it; }
    }
    // 清理越界子弹
    for (auto it = playerBullets.begin(); it != playerBullets.end(); ) {
        if ((*it)->y < -20 || (*it)->y > WINDOW_HEIGHT + 20) {
            delete *it;
            it = playerBullets.erase(it);
        } else { ++it; }
    }
    for (auto it = enemyBullets.begin(); it != enemyBullets.end(); ) {
        if ((*it)->y < -20 || (*it)->y > WINDOW_HEIGHT + 20) {
            delete *it;
            it = enemyBullets.erase(it);
        } else { ++it; }
    }
    // 清理越界道具
    for (auto it = items.begin(); it != items.end(); ) {
        if (!(*it)->alive || (*it)->y > WINDOW_HEIGHT + 20) {
            delete *it;
            it = items.erase(it);
        } else { ++it; }
    }
    // 清理过期粒子
    particles.erase(
        std::remove_if(particles.begin(), particles.end(),
            [](const Particle& p) { return !p.alive; }),
        particles.end()
    );
}

void Game::activateBomb() {
    if (!player || player->bombCount <= 0) return;
    player->bombCount--;

    // 检查是否炸到了Boss
    for (auto* e : enemies) {
        if (e->getType() == EnemyType::BOSS && e->alive) {
            bossDefeated = true;
            if (currentLevel >= LEVEL_COUNT) {
                state = GameState::VICTORY;
            }
        }
    }

    // 清除所有敌机和敌机子弹
    for (auto* e : enemies) {
        spawnParticles(e->centerX(), e->centerY(), EXPLOSION_PARTICLE_COUNT, RGB(255, 150, 0));
        if (e->getType() != EnemyType::BOSS) {
            score += e->scoreValue;
        }
        delete e;
    }
    enemies.clear();
    for (auto* b : enemyBullets) delete b;
    enemyBullets.clear();

    // 屏幕震动
    shakeTimer = 15;
    shakeIntensity = 5;

    // 白色闪光粒子
    for (int i = 0; i < 50; i++) {
        Particle p;
        p.x = (float)(rand() % WINDOW_WIDTH);
        p.y = (float)(rand() % WINDOW_HEIGHT);
        p.vx = 0; p.vy = 0;
        p.lifetime = 10 + rand() % 10;
        p.maxLifetime = p.lifetime;
        p.color = RGB(255, 255, 200);
        p.alive = true;
        particles.push_back(p);
    }
}

// ---------- 碰撞检测 ----------

void Game::collide() {
    if (state != GameState::PLAYING || !player) return;
    checkBulletEnemyCollisions();
    checkEnemyBulletPlayerCollisions();
    checkEnemyPlayerCollisions();
    checkItemPlayerCollisions();
}

void Game::checkBulletEnemyCollisions() {
    for (auto* bullet : playerBullets) {
        if (!bullet->alive) continue;
        for (auto* enemy : enemies) {
            if (!enemy->alive) continue;
            if (checkCollision(*bullet, *enemy)) {
                bullet->alive = false;
                enemy->hp -= bullet->damage;
                if (enemy->hp <= 0) {
                    enemy->alive = false;
                    score += enemy->scoreValue;
                    enemiesKilledThisLevel++;

                    bool isBoss = (enemy->getType() == EnemyType::BOSS);

                    if (isBoss) {
                        // Boss 爆炸 — 3波
                        for (int w = 0; w < BOSS_EXPLOSION_WAVES; w++) {
                            spawnParticles(enemy->centerX(), enemy->centerY(),
                                EXPLOSION_PARTICLE_COUNT * 3, RGB(255, 50 + w * 50, 0));
                        }
                        // Boss 必掉3个道具
                        for (int d = 0; d < 3; d++) {
                            float r2 = (float)(rand() % 100) / 100.0f;
                            ItemType it;
                            if (r2 < 0.40f)      it = ItemType::FIREPOWER;
                            else if (r2 < 0.60f) it = ItemType::HEALTH;
                            else if (r2 < 0.80f) it = ItemType::BOMB;
                            else                 it = ItemType::SHIELD;
                            items.push_back(new Item(
                                enemy->centerX() - ITEM_WIDTH / 2.0f + (d - 1) * 30.0f,
                                enemy->y, it));
                        }
                        shakeTimer = 25;
                        shakeIntensity = 6;
                        bossDefeated = true;

                        // 是否最终通关
                        if (currentLevel >= LEVEL_COUNT) {
                            state = GameState::VICTORY;
                        }
                    } else {
                        spawnParticles(enemy->centerX(), enemy->centerY(),
                            EXPLOSION_PARTICLE_COUNT, RGB(255, 100, 30));

                        // 道具掉落概率
                        if ((float)(rand() % 100) / 100.0f < ITEM_DROP_CHANCE) {
                            float r2 = (float)(rand() % 100) / 100.0f;
                            ItemType it;
                            if (r2 < 0.40f)      it = ItemType::FIREPOWER;
                            else if (r2 < 0.60f) it = ItemType::HEALTH;
                            else if (r2 < 0.80f) it = ItemType::BOMB;
                            else                 it = ItemType::SHIELD;
                            items.push_back(new Item(enemy->centerX() - ITEM_WIDTH / 2.0f,
                                                     enemy->y, it));
                        }
                    }
                }
                break;
            }
        }
    }
}

void Game::checkEnemyBulletPlayerCollisions() {
    for (auto* bullet : enemyBullets) {
        if (!bullet->alive) continue;
        if (checkCollision(*bullet, *player)) {
            bullet->alive = false;
            player->takeDamage();
            spawnParticles(player->centerX(), player->centerY(), 10, RGB(255, 200, 50));
            shakeTimer = 8;
            shakeIntensity = 3;
            if (player->lives <= 0) {
                state = GameState::GAME_OVER;
            }
            return;
        }
    }
}

void Game::checkEnemyPlayerCollisions() {
    if (player->isInvincible()) return;
    for (auto* enemy : enemies) {
        if (!enemy->alive) continue;
        if (checkCollision(*player, *enemy)) {
            enemy->alive = false;
            player->takeDamage();
            spawnParticles(enemy->centerX(), enemy->centerY(),
                EXPLOSION_PARTICLE_COUNT / 2, RGB(255, 100, 30));
            spawnParticles(player->centerX(), player->centerY(), 10, RGB(255, 200, 50));
            shakeTimer = 12;
            shakeIntensity = 4;
            if (player->lives <= 0) {
                state = GameState::GAME_OVER;
            }
            return;
        }
    }
}

void Game::checkItemPlayerCollisions() {
    for (auto* it : items) {
        if (!it->alive) continue;
        if (checkCollision(*player, *it)) {
            it->alive = false;
            switch (it->itemType) {
            case ItemType::FIREPOWER: player->increaseFirepower(); break;
            case ItemType::HEALTH:    player->addLife();           break;
            case ItemType::BOMB:      player->addBomb();           break;
            case ItemType::SHIELD:    player->activateShield();    break;
            }
            // 拾取特效
            for (int i = 0; i < 8; i++) {
                Particle p;
                p.x = it->centerX();
                p.y = it->centerY();
                float ang = (float)i * 3.14159f * 2.0f / 8.0f;
                p.vx = cosf(ang) * 2.0f;
                p.vy = sinf(ang) * 2.0f;
                p.lifetime = 15;
                p.maxLifetime = 15;
                p.color = RGB(255, 255, 100);
                p.alive = true;
                particles.push_back(p);
            }
        }
    }
}

// ---------- 粒子特效 ----------

void Game::spawnParticles(float cx, float cy, int count, COLORREF color) {
    for (int i = 0; i < count && (int)particles.size() < PARTICLE_MAX; i++) {
        Particle p;
        p.x = cx;
        p.y = cy;
        float ang = (float)(rand() % 628) / 100.0f;
        float spd = 0.5f + (float)(rand() % 100) / 40.0f;  // 0.5 ~ 3.0
        p.vx = cosf(ang) * spd;
        p.vy = sinf(ang) * spd;
        p.lifetime = 12 + rand() % EXPLOSION_PARTICLE_LIFE;
        p.maxLifetime = p.lifetime;
        p.color = color;
        p.alive = true;
        particles.push_back(p);
    }
}

// ---------- 关卡管理 ----------

void Game::advanceLevel() {
    // Boss 出现：清除普通敌机，生成 Boss
    bossSpawned = true;
    for (auto* e : enemies) {
        if (e->getType() != EnemyType::BOSS) {
            spawnParticles(e->centerX(), e->centerY(), 5, RGB(255, 200, 50));
            delete e;
        }
    }
    enemies.erase(
        std::remove_if(enemies.begin(), enemies.end(),
            [](Enemy* e) { return e->getType() != EnemyType::BOSS; }),
        enemies.end()
    );
    for (auto* b : enemyBullets) delete b;
    enemyBullets.clear();

    // 生成Boss（使用当前关卡难度）
    Boss* boss = new Boss(WINDOW_WIDTH / 2.0f - BOSS_WIDTH / 2.0f, currentLevel);
    enemies.push_back(boss);

    // Boss 直接登场（入场动画自然播放），不切换关卡
    // state 保持 PLAYING
}

void Game::startNextLevel() {
    // 进入下一关（在 Boss 被击败或普通关卡推进时调用）
    currentLevel++;
    state = GameState::LEVEL_TRANSITION;
    stateTimer = LEVEL_TRANSITION_FRAMES;
    enemySpawnTimer = 60;

    // 难度递增，scoreToNextLevel 基于当前分数累加，避免跳关
    switch (currentLevel) {
    case 1:
        scoreToNextLevel = score + 500;
        spawnInterval = 40.0f;
        fastEnemyChance = 0.10f;
        shootingEnemyChance = 0.00f;
        break;
    case 2:
        scoreToNextLevel = score + 800;
        spawnInterval = 28.0f;
        fastEnemyChance = 0.25f;
        shootingEnemyChance = 0.10f;
        break;
    case 3:
        scoreToNextLevel = score + 1200;
        spawnInterval = 20.0f;
        fastEnemyChance = 0.30f;
        shootingEnemyChance = 0.20f;
        break;
    case 4:
        scoreToNextLevel = score + 1600;
        spawnInterval = 15.0f;
        fastEnemyChance = 0.35f;
        shootingEnemyChance = 0.30f;
        break;
    default:  // 5+
        scoreToNextLevel = score + 2000;
        spawnInterval = (std::max)(8.0f, spawnInterval - 3.0f);
        fastEnemyChance = (std::min)(0.50f, fastEnemyChance + 0.05f);
        shootingEnemyChance = (std::min)(0.40f, shootingEnemyChance + 0.05f);
        break;
    }

    bossSpawned = false;
    bossDefeated = false;
}

// ---------- 排行榜 ----------

void Game::loadLeaderboard() {
    leaderboard.clear();

    // 使用 EXE 所在目录的完整路径
    char path[MAX_PATH];
    GetModuleFileNameA(nullptr, path, MAX_PATH);
    char* lastSlash = strrchr(path, '\\');
    if (lastSlash) *(lastSlash + 1) = '\0';
    strcat_s(path, "scores.dat");

    HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return;

    DWORD count = 0, bytesRead;
    if (!ReadFile(hFile, &count, sizeof(DWORD), &bytesRead, nullptr) || count > 50) {
        CloseHandle(hFile);
        return;  // 文件损坏，跳过
    }

    for (DWORD i = 0; i < count; i++) {
        ScoreEntry entry;
        memset(&entry, 0, sizeof(entry));
        if (!ReadFile(hFile, &entry, sizeof(ScoreEntry), &bytesRead, nullptr)) break;
        leaderboard.push_back(entry);
    }
    CloseHandle(hFile);
    if (!leaderboard.empty()) highScore = leaderboard[0].score;
}

void Game::saveLeaderboard() {
    char path[MAX_PATH];
    GetModuleFileNameA(nullptr, path, MAX_PATH);
    char* lastSlash = strrchr(path, '\\');
    if (lastSlash) *(lastSlash + 1) = '\0';
    strcat_s(path, "scores.dat");

    HANDLE hFile = CreateFileA(path, GENERIC_WRITE, 0,
        nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return;

    DWORD count = (DWORD)leaderboard.size(), bytesWritten;
    WriteFile(hFile, &count, sizeof(DWORD), &bytesWritten, nullptr);
    for (const auto& entry : leaderboard)
        WriteFile(hFile, &entry, sizeof(ScoreEntry), &bytesWritten, nullptr);
    CloseHandle(hFile);
}

void Game::addScoreEntry(const char* name) {
    char date[11];
    SYSTEMTIME st; GetLocalTime(&st);
    sprintf_s(date, "%04d-%02d-%02d", st.wYear, st.wMonth, st.wDay);
    ScoreEntry entry;
    memset(&entry, 0, sizeof(entry));
    strncpy_s(entry.name, name, NAME_MAX_LEN - 1);
    entry.score = score; entry.level = currentLevel;
    strncpy_s(entry.date, date, 10);
    auto it = leaderboard.begin();
    while (it != leaderboard.end() && it->score > score) ++it;
    leaderboard.insert(it, entry);
    if (leaderboard.size() > LEADERBOARD_MAX) leaderboard.pop_back();
    if (!leaderboard.empty()) highScore = leaderboard[0].score;
    saveLeaderboard();
}

bool Game::isHighScore() const {
    if (leaderboard.size() < LEADERBOARD_MAX) return true;
    return score > leaderboard.back().score;
}

// ============================================================
// 渲染
// ============================================================

void Game::render() {
    HBRUSH blackBrush = CreateSolidBrush(RGB(0, 0, 0));
    RECT rc = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    FillRect(backDC, &rc, blackBrush);
    DeleteObject(blackBrush);

    switch (state) {
    case GameState::MENU:       renderMenu();           break;
    case GameState::SETTINGS:   renderSettings();       break;
    case GameState::PLAYING:
    case GameState::PAUSED:
    case GameState::LEVEL_TRANSITION:
        renderBackground();
        renderParticles();
        renderItems();
        renderEnemies();
        // Boss血条
        for (auto* e : enemies) {
            if (e->getType() == EnemyType::BOSS && e->alive) {
                static_cast<Boss*>(e)->renderHPBar(backDC);
            }
        }
        renderBullets();
        if (player) player->render(backDC);
        renderHUD();
        if (state == GameState::PAUSED) renderPauseMenu();
        if (state == GameState::LEVEL_TRANSITION) renderLevelTransition();
        break;
    case GameState::GAME_OVER:
    case GameState::VICTORY:    renderGameOver();       break;
    case GameState::LEADERBOARD: renderLeaderboard();    break;
    case GameState::NAME_ENTRY:  renderNameEntry();      break;
    }

    BitBlt(frontDC, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, backDC, 0, 0, SRCCOPY);
}

// ---- 各实体渲染 ----

void Game::renderBackground() {
    for (const auto& s : stars) {
        int b = s.brightness;
        COLORREF c = RGB(b, b, b);
        SetPixel(backDC, (int)s.x, (int)s.y, c);
        if (s.speed > 1.2f) {
            SetPixel(backDC, (int)s.x + 1, (int)s.y, c);
            SetPixel(backDC, (int)s.x, (int)s.y + 1, c);
        }
    }
}

void Game::renderPlayer() {
    if (player) player->render(backDC);
}

void Game::renderEnemies() {
    for (auto* e : enemies) if (e->alive) e->render(backDC);
}

void Game::renderBullets() {
    for (auto* b : playerBullets) if (b->alive) b->render(backDC);
    for (auto* b : enemyBullets) if (b->alive) b->render(backDC);
}

void Game::renderItems() {
    for (auto* it : items) if (it->alive) it->render(backDC);
}

void Game::renderParticles() {
    for (const auto& p : particles) {
        if (!p.alive) continue;
        float alpha = (float)p.lifetime / p.maxLifetime;
        int r = GetRValue(p.color);
        int g = GetGValue(p.color);
        int b = GetBValue(p.color);
        r = (int)(r * alpha); g = (int)(g * alpha); b = (int)(b * alpha);
        SetPixel(backDC, (int)p.x, (int)p.y, RGB(r, g, b));
        // 较大粒子画2x2
        if (p.maxLifetime > 15) {
            SetPixel(backDC, (int)p.x + 1, (int)p.y, RGB(r, g, b));
            SetPixel(backDC, (int)p.x, (int)p.y + 1, RGB(r, g, b));
        }
    }
}

void Game::renderHUD() {
    SetBkMode(backDC, TRANSPARENT);

    // 分数
    char buf[128];
    SetTextColor(backDC, RGB(255, 255, 255));
    sprintf_s(buf, "Score: %d", score);
    TextOutA(backDC, 10, 8, buf, (int)strlen(buf));

    // 生命值
    SetTextColor(backDC, RGB(255, 50, 50));
    sprintf_s(buf, "Lives: ");
    TextOutA(backDC, 10, 28, buf, (int)strlen(buf));
    for (int i = 0; i < player->lives; i++) {
        TextOutA(backDC, 70 + i * 18, 28, "O", 1);
    }

    // 炸弹数
    SetTextColor(backDC, RGB(255, 150, 0));
    sprintf_s(buf, "Bombs: %d", player->bombCount);
    TextOutA(backDC, 10, 48, buf, (int)strlen(buf));

    // 火力等级
    SetTextColor(backDC, RGB(255, 200, 0));
    sprintf_s(buf, "Power: Lv.%d", player->firepowerLevel);
    TextOutA(backDC, 10, 68, buf, (int)strlen(buf));

    // 护盾状态
    if (player->shieldActive) {
        SetTextColor(backDC, RGB(0, 180, 255));
        sprintf_s(buf, "Shield: %d", player->shieldTimer / 30);
        TextOutA(backDC, 10, 88, buf, (int)strlen(buf));
    }

    // 关卡
    SetTextColor(backDC, RGB(200, 200, 200));
    sprintf_s(buf, "Level: %d", currentLevel);
    TextOutA(backDC, WINDOW_WIDTH - 120, 8, buf, (int)strlen(buf));

    // 下一关分数
    if (!bossSpawned) {
        sprintf_s(buf, "Next: %d", scoreToNextLevel);
        TextOutA(backDC, WINDOW_WIDTH - 120, 28, buf, (int)strlen(buf));
    } else {
        SetTextColor(backDC, RGB(255, 50, 50));
        TextOutA(backDC, WINDOW_WIDTH - 120, 28, "BOSS!", 5);
    }

    // 难度指示
    sprintf_s(buf, "Enemies: %d", (int)enemies.size());
    SetTextColor(backDC, RGB(150, 150, 150));
    TextOutA(backDC, WINDOW_WIDTH - 120, 48, buf, (int)strlen(buf));

    // 最高分
    sprintf_s(buf, "Best: %d", highScore);
    TextOutA(backDC, WINDOW_WIDTH - 120, 68, buf, (int)strlen(buf));
}

// ---- 菜单渲染 ----

void Game::renderMenu() {
    SetBkMode(backDC, TRANSPARENT);

    HFONT titleFont = CreateFontA(48, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
    HFONT oldFont = (HFONT)SelectObject(backDC, titleFont);

    SetTextColor(backDC, RGB(255, 200, 0));
    RECT titleRC = { 0, 60, WINDOW_WIDTH, 140 };
    DrawTextA(backDC, "PLANE WAR", -1, &titleRC, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    HFONT subFont = CreateFontA(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
    SelectObject(backDC, subFont); DeleteObject(titleFont);

    SetTextColor(backDC, RGB(150, 150, 150));
    RECT subRC = { 0, 140, WINDOW_WIDTH, 170 };
    DrawTextA(backDC, "C++ Course Project", -1, &subRC, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    DeleteObject(subFont);

    // 菜单选项
    HFONT menuFont = CreateFontA(24, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
    SelectObject(backDC, menuFont);

    const char* menuTexts[] = { "1. Start Game", "2. Leaderboard", "3. Settings", "4. Exit" };
    for (int i = 0; i < 4; i++) {
        COLORREF color = (int)menuSelection == i ? RGB(255, 255, 100) : RGB(180, 180, 180);
        SetTextColor(backDC, color);
        RECT itemRC = { 0, 260 + i * 45, WINDOW_WIDTH, 300 + i * 45 };
        DrawTextA(backDC, menuTexts[i], -1, &itemRC, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    HFONT hintFont = CreateFontA(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
    SelectObject(backDC, hintFont); DeleteObject(menuFont);

    SetTextColor(backDC, RGB(100, 100, 100));
    RECT hintRC = { 0, 430, WINDOW_WIDTH, 465 };
    DrawTextA(backDC, "Arrows/WASD Move | Space Shoot | B Bomb | P Pause | ESC Menu",
        -1, &hintRC, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    DeleteObject(hintFont);
    SelectObject(backDC, oldFont);
}

void Game::renderSettings() {
    SetBkMode(backDC, TRANSPARENT);

    HFONT titleFont = CreateFontA(32, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
    HFONT oldFont = (HFONT)SelectObject(backDC, titleFont);

    SetTextColor(backDC, RGB(255, 200, 0));
    RECT trc = { 0, 40, WINDOW_WIDTH, 90 };
    DrawTextA(backDC, "SETTINGS - Resolution", -1, &trc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    HFONT optFont = CreateFontA(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Consolas");
    SelectObject(backDC, optFont);
    DeleteObject(titleFont);

    // 列出所有分辨率
    for (int i = 0; i < RESOLUTION_COUNT; i++) {
        COLORREF color = (settingsSelection == i) ? RGB(255, 255, 100) : RGB(180, 180, 180);
        if (i == currentResolution) color = (settingsSelection == i) ? RGB(0, 255, 100) : RGB(100, 200, 100);

        SetTextColor(backDC, color);
        RECT rc = { 100, 140 + i * 55, WINDOW_WIDTH - 100, 185 + i * 55 };
        char buf[64];
        sprintf_s(buf, "%s  %s", RESOLUTIONS[i].label,
            i == currentResolution ? "(current)" : "");
        DrawTextA(backDC, buf, -1, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }

    // 提示
    HFONT hintFont = CreateFontA(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
    SelectObject(backDC, hintFont);
    DeleteObject(optFont);

    SetTextColor(backDC, RGB(130, 130, 130));
    RECT rc6 = { 0, 400, WINDOW_WIDTH, 440 };
    DrawTextA(backDC, "Arrows = Select | Enter = Apply | ESC = Back", -1,
        &rc6, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(backDC, oldFont);
    DeleteObject(hintFont);
}

void Game::renderPauseMenu() {
    SetBkMode(backDC, TRANSPARENT);
    for (int y = 0; y < WINDOW_HEIGHT; y += 3) {
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
        HPEN oldPen = (HPEN)SelectObject(backDC, pen);
        MoveToEx(backDC, 0, y, nullptr);
        LineTo(backDC, WINDOW_WIDTH, y);
        SelectObject(backDC, oldPen);
        DeleteObject(pen);
    }

    HFONT font = CreateFontA(36, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
    HFONT oldFont = (HFONT)SelectObject(backDC, font);
    SetTextColor(backDC, RGB(255, 255, 100));
    RECT rc = { 0, 150, WINDOW_WIDTH, 220 };
    DrawTextA(backDC, "PAUSED", -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    HFONT subFont = CreateFontA(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
    SelectObject(backDC, subFont); DeleteObject(font);
    SetTextColor(backDC, RGB(200, 200, 200));
    RECT rc2 = { 0, 250, WINDOW_WIDTH, 300 };
    DrawTextA(backDC, "P = Resume | ESC = Back to Menu", -1,
        &rc2, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(backDC, oldFont);
    DeleteObject(subFont);
}

void Game::renderGameOver() {
    SetBkMode(backDC, TRANSPARENT);
    bool isVict = (state == GameState::VICTORY);

    HFONT font = CreateFontA(40, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
    HFONT oldFont = (HFONT)SelectObject(backDC, font);
    SetTextColor(backDC, isVict ? RGB(255, 200, 0) : RGB(255, 50, 50));
    RECT rc = { 0, 80, WINDOW_WIDTH, 150 };
    DrawTextA(backDC, isVict ? "VICTORY!" : "GAME OVER", -1,
        &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    HFONT subFont = CreateFontA(22, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
    SelectObject(backDC, subFont); DeleteObject(font);

    char buf[128];
    SetTextColor(backDC, RGB(255, 255, 255));
    sprintf_s(buf, "Final Score: %d", score);
    RECT rc2 = { 0, 180, WINDOW_WIDTH, 215 };
    DrawTextA(backDC, buf, -1, &rc2, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    sprintf_s(buf, "Level Reached: %d", currentLevel);
    RECT rc3 = { 0, 215, WINDOW_WIDTH, 250 };
    DrawTextA(backDC, buf, -1, &rc3, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    sprintf_s(buf, "Enemies Destroyed: %d", enemiesKilledThisLevel);
    RECT rc4 = { 0, 250, WINDOW_WIDTH, 285 };
    DrawTextA(backDC, buf, -1, &rc4, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // 检查是否能上榜（GAME_OVER 或 VICTORY）
    if ((state == GameState::GAME_OVER || state == GameState::VICTORY) && isHighScore()) {
        SetTextColor(backDC, RGB(255, 200, 0));
        RECT rc6 = { 0, 310, WINDOW_WIDTH, 340 };
        DrawTextA(backDC, "NEW HIGH SCORE! Press Enter to record", -1,
            &rc6, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    SetTextColor(backDC, RGB(180, 180, 180));
    RECT rc5 = { 0, 360, WINDOW_WIDTH, 400 };
    DrawTextA(backDC, "Press Enter to continue", -1,
        &rc5, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(backDC, oldFont);
    DeleteObject(subFont);
}

void Game::renderLeaderboard() {
    SetBkMode(backDC, TRANSPARENT);
    HFONT titleFont = CreateFontA(32, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
    HFONT oldFont = (HFONT)SelectObject(backDC, titleFont);
    SetTextColor(backDC, RGB(255, 200, 0));
    RECT trc = { 0, 20, WINDOW_WIDTH, 65 };
    DrawTextA(backDC, "LEADERBOARD - Top 10", -1, &trc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    HFONT listFont = CreateFontA(17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Consolas");
    SelectObject(backDC, listFont); DeleteObject(titleFont);

    SetTextColor(backDC, RGB(140, 140, 140));
    RECT hrc = { 40, 75, WINDOW_WIDTH - 40, 100 };
    DrawTextA(backDC, "RANK  NAME            SCORE      LEVEL  DATE",
        -1, &hrc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    HPEN lp = CreatePen(PS_SOLID, 1, RGB(80, 80, 80));
    HPEN op = (HPEN)SelectObject(backDC, lp);
    MoveToEx(backDC, 40, 100, nullptr); LineTo(backDC, WINDOW_WIDTH - 40, 100);
    SelectObject(backDC, op); DeleteObject(lp);

    char buf[128];
    for (size_t i = 0; i < leaderboard.size(); i++) {
        const auto& e = leaderboard[i];
        COLORREF c = (i == 0) ? RGB(255, 215, 0) : (i == 1) ? RGB(192, 192, 192) :
                     (i == 2) ? RGB(205, 127, 50) : RGB(190, 190, 190);
        SetTextColor(backDC, c);
        sprintf_s(buf, "%-5d %-15s %-10d %-6d %s",
            (int)(i + 1), e.name, e.score, e.level, e.date);
        RECT ir = { 40, 105 + (int)i * 26, WINDOW_WIDTH - 40, 131 + (int)i * 26 };
        DrawTextA(backDC, buf, -1, &ir, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }

    if (leaderboard.empty()) {
        SetTextColor(backDC, RGB(140, 140, 140));
        RECT er = { 0, 140, WINDOW_WIDTH, 180 };
        DrawTextA(backDC, "No records yet. Play a game!", -1, &er, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    SetTextColor(backDC, RGB(120, 120, 120));
    RECT hrc2 = { 0, 435, WINDOW_WIDTH, 465 };
    DrawTextA(backDC, "Press ESC to return to Menu", -1, &hrc2, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(backDC, oldFont);
    DeleteObject(listFont);
}

void Game::renderNameEntry() {
    SetBkMode(backDC, TRANSPARENT);

    // 半透明背景
    for (int y = 0; y < WINDOW_HEIGHT; y += 3) {
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(10, 10, 30));
        HPEN oldPen = (HPEN)SelectObject(backDC, pen);
        MoveToEx(backDC, 0, y, nullptr);
        LineTo(backDC, WINDOW_WIDTH, y);
        SelectObject(backDC, oldPen);
        DeleteObject(pen);
    }

    HFONT font = CreateFontA(30, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
    HFONT oldFont = (HFONT)SelectObject(backDC, font);

    SetTextColor(backDC, RGB(255, 200, 0));
    RECT rc = { 0, 80, WINDOW_WIDTH, 130 };
    DrawTextA(backDC, "NEW HIGH SCORE!", -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    HFONT subFont = CreateFontA(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Consolas");
    SelectObject(backDC, subFont);
    DeleteObject(font);

    char buf[128];
    sprintf_s(buf, "Score: %d   Level: %d", score, currentLevel);
    SetTextColor(backDC, RGB(255, 255, 255));
    RECT rc2 = { 0, 150, WINDOW_WIDTH, 185 };
    DrawTextA(backDC, buf, -1, &rc2, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SetTextColor(backDC, RGB(200, 200, 200));
    RECT rc3 = { 0, 210, WINDOW_WIDTH, 245 };
    DrawTextA(backDC, "Enter your name (3 letters):", -1, &rc3, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // 名字输入框
    char nameDisplay[32] = {};
    if (nameLength > 0) {
        strncpy_s(nameDisplay, nameBuffer, nameLength);
    }
    strcat_s(nameDisplay, "_");  // 光标

    HFONT nameFont = CreateFontA(36, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Consolas");
    SelectObject(backDC, nameFont);
    DeleteObject(subFont);

    SetTextColor(backDC, RGB(0, 255, 255));
    RECT rc4 = { 0, 260, WINDOW_WIDTH, 310 };
    DrawTextA(backDC, nameDisplay, -1, &rc4, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SetTextColor(backDC, RGB(150, 150, 150));
    RECT rc5 = { 0, 350, WINDOW_WIDTH, 390 };
    DrawTextA(backDC, "A-Z keys | Enter = Confirm | ESC = Skip", -1,
        &rc5, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(backDC, oldFont);
    DeleteObject(nameFont);
}

void Game::renderLevelTransition() {
    SetBkMode(backDC, TRANSPARENT);
    HFONT font = CreateFontA(42, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
    HFONT oldFont = (HFONT)SelectObject(backDC, font);
    char buf[64];
    sprintf_s(buf, "LEVEL %d", currentLevel);
    SetTextColor(backDC, RGB(255, 200, 0));
    RECT rc = { 0, 170, WINDOW_WIDTH, 250 };
    DrawTextA(backDC, buf, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(backDC, oldFont);
    DeleteObject(font);
}

void Game::renderBossHPBar() {}
