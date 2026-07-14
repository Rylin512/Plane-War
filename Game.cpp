#include "Game.h"
#include "Player.h"
#include "Enemy.h"
#include "Boss.h"
#include "Bullet.h"
#include "Item.h"
#include "Resource.h"
#include <cstdio>
#include <cmath>
#include <cwchar>
#include <algorithm>

// MinGW: windows.h 定义了 max/min 宏，干扰 std::max/min
#undef max
#undef min

using namespace Gdiplus;

// ── 全局可变状态（Config.h 中 extern 声明，此处统一定义） ──
int   WINDOW_WIDTH  = 640;
int   WINDOW_HEIGHT = 480;
float GAME_SCALE    = 1.0f;
int   BG_ALPHA      = 75;

static Game* g_game = nullptr;

// 前向声明（宽字符转换辅助，栈缓冲区避免 wstring ABI 问题）
static const wchar_t* widen(const char* s);

// ========== 窗口过程 ==========

LRESULT CALLBACK Game::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_DESTROY: PostQuitMessage(0); return 0;
    case WM_KEYDOWN:
        if (g_game && wParam == VK_ESCAPE) {
            if (g_game->state == GameState::HELP) { g_game->state = GameState::PLAYING; }
            else if (g_game->state == GameState::PLAYING) g_game->state = GameState::PAUSED;
            else if (g_game->state == GameState::PAUSED) {
                delete g_game->player; g_game->player = nullptr;
                for (auto* e : g_game->enemies) delete e;
                for (auto* b : g_game->playerBullets) delete b;
                for (auto* b : g_game->enemyBullets) delete b;
                for (auto* i : g_game->items) delete i;
                g_game->enemies.clear(); g_game->playerBullets.clear();
                g_game->enemyBullets.clear(); g_game->items.clear();
                g_game->particles.clear();
                g_game->state = GameState::MENU;
            }
        }
        if (g_game && wParam == 'H') {
            if (g_game->state == GameState::PLAYING) g_game->state = GameState::HELP;
            else if (g_game->state == GameState::HELP) g_game->state = GameState::PLAYING;
        }
        return 0;
    case WM_ACTIVATE:
        if (g_game && g_game->state == GameState::PLAYING && LOWORD(wParam) == WA_INACTIVE)
            g_game->state = GameState::PAUSED;
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// ========== 构造 / 析构 ==========

Game::Game()
    : hwnd(nullptr), frontDC(nullptr), bgImage(nullptr), uiFrame(nullptr), bgAlpha(BG_ALPHA), fullscreen(false)
    , keyLeft(false), keyRight(false), keyUp(false), keyDown(false)
    , keySpace(false), keyBomb(false), keyPause(false), keyShift(false), keyCtrl(false), keyHelp(false)
    , spacePressed(false), frameCount(0)
    , state(GameState::MENU), menuSelection(MenuOption::START_GAME)
    , settingsSelection(0), currentResolution(0), stateTimer(0)
    , shakeTimer(0), shakeIntensity(0)
    , currentLevel(0), scoreToNextLevel(500), spawnInterval(40), fastEnemyChance(0.1f), shootingEnemyChance(0)
    , enemySpawnTimer(0), enemiesKilledThisLevel(0)
    , bossSpawned(false), bossDefeated(false)
    , score(0), highScore(0), player(nullptr), nameLength(0)
    , deltaTime(0), menuCooldown(0)
{
    g_game = this;
    memset(nameBuffer, 0, sizeof(nameBuffer));
    QueryPerformanceFrequency(&qpcFreq);
    QueryPerformanceCounter(&qpcPrev);
    srand((unsigned int)time(nullptr));
}

Game::~Game() { shutdown(); }

// ========== 窗口创建 ==========

bool Game::createWindow(HINSTANCE hInst) {
    WNDCLASSA wc = {};
    wc.lpfnWndProc = WndProc; wc.hInstance = hInst;
    wc.lpszClassName = "PlaneWarGDI+";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    if (!RegisterClassA(&wc)) return false;

    RECT rc = {0,0,WINDOW_WIDTH,WINDOW_HEIGHT};
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX, FALSE);
    hwnd = CreateWindowExA(0, "PlaneWarGDI+", "Plane War — GDI+ Edition",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, rc.right-rc.left, rc.bottom-rc.top,
        nullptr, nullptr, hInst, nullptr);
    if (!hwnd) return false;

    frontDC = GetDC(hwnd);
    ShowWindow(hwnd, SW_SHOW);
    return true;
}

bool Game::init(HINSTANCE hInst) {
    WINDOW_WIDTH  = RESOLUTIONS[currentResolution].width;
    WINDOW_HEIGHT = RESOLUTIONS[currentResolution].height;
    GAME_SCALE    = WINDOW_HEIGHT / 480.0f;  // 全局缩放

    if (!createWindow(hInst)) return false;

    // 加载素材
    bgImage = Resource::LoadImage(Resource::BG_CIRCUIT);
    uiFrame = Resource::LoadImage(Resource::UI_FRAME);

    loadLeaderboard();
    resetStars();
    return true;
}

void Game::applyResolution(int idx) {
    if (idx < 0 || idx >= RESOLUTION_COUNT || idx == currentResolution) return;

    // 保存旧尺寸用于比例缩放实体位置
    float oldW = (float)WINDOW_WIDTH, oldH = (float)WINDOW_HEIGHT;

    currentResolution = idx;
    WINDOW_WIDTH  = RESOLUTIONS[idx].width;
    WINDOW_HEIGHT = RESOLUTIONS[idx].height;
    GAME_SCALE    = WINDOW_HEIGHT / 480.0f;
    fullscreen     = RESOLUTIONS[idx].fullscreen;

    if (hwnd) {
        if (fullscreen) {
            SetWindowLongPtr(hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
            SetWindowPos(hwnd, HWND_TOP, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, SWP_SHOWWINDOW);
        } else {
            SetWindowLongPtr(hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX);
            RECT rc = {0,0,WINDOW_WIDTH,WINDOW_HEIGHT};
            AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX, FALSE);
            SetWindowPos(hwnd, nullptr, 0, 0, rc.right-rc.left, rc.bottom-rc.top,
                         SWP_NOMOVE | SWP_NOZORDER | SWP_SHOWWINDOW);
            int sw = GetSystemMetrics(SM_CXSCREEN), sh = GetSystemMetrics(SM_CYSCREEN);
            SetWindowPos(hwnd, nullptr, (sw-(rc.right-rc.left))/2, (sh-(rc.bottom-rc.top))/2,
                         0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
        // ★ 关键修复：重获取 frontDC，使其裁剪区域匹配新窗口尺寸
        if (frontDC) { ReleaseDC(hwnd, frontDC); }
        frontDC = GetDC(hwnd);
    }

    resetStars();

    // 比例缩放因子：新旧坐标系转换
    float sx = (float)WINDOW_WIDTH  / oldW;
    float sy = (float)WINDOW_HEIGHT / oldH;

    if (player) {
        // 更新碰撞盒尺寸为新 GAME_SCALE
        player->width  = (int)(PLAYER_WIDTH  * GAME_SCALE);
        player->height = (int)(PLAYER_HEIGHT * GAME_SCALE);
        // 按比例缩放位置到新坐标空间
        player->x = player->x * sx;
        player->y = player->y * sy;
        player->clampToScreen();
    }
    // 缩放现有实体位置（碰撞盒不缩放，新生成即用新 GAME_SCALE）
    for (auto* e : enemies)     { e->x *= sx; e->y *= sy; }
    for (auto* b : playerBullets){ b->x *= sx; b->y *= sy; }
    for (auto* b : enemyBullets) { b->x *= sx; b->y *= sy; }
    for (auto* it: items)       { it->x *= sx; it->y *= sy; }
    for (auto& p : particles)   { p.x *= sx; p.y *= sy; }
}

// ========== 星空 ==========

void Game::resetStars() {
    stars.clear();
    int cnt = (int)(WINDOW_WIDTH * WINDOW_HEIGHT / (2000.0f / GAME_SCALE));
    for (int i = 0; i < cnt; i++) {
        Star s;
        s.x = (float)(rand() % WINDOW_WIDTH);
        s.y = (float)(rand() % WINDOW_HEIGHT);
        s.size = (rand()%100)<8 ? 3 : ((rand()%100)<25 ? 2 : 1);
        s.speed = 0.3f + (rand()%150)/100.0f;
        s.brightness = 80 + rand()%176;
        s.twinkleTimer = rand()%120;
        s.twinklePhase = rand()%120;
        int cr = rand()%100;
        if      (cr < 5)  s.color = Color(255, 140, 160, 255);
        else if (cr < 10) s.color = Color(255, 255, 220, 140);
        else if (cr < 13) s.color = Color(255, 255, 160, 140);
        else              s.color = Color(255, 200, 210, 255);
        stars.push_back(s);
    }
}

// ========== 关闭 ==========

void Game::shutdown() {
    delete player; player = nullptr;
    for (auto* e : enemies) delete e;
    for (auto* b : playerBullets) delete b;
    for (auto* b : enemyBullets) delete b;
    for (auto* i : items) delete i;
    enemies.clear(); playerBullets.clear(); enemyBullets.clear(); items.clear();
    particles.clear();
    delete bgImage; bgImage = nullptr;
    delete uiFrame; uiFrame = nullptr;
    if (frontDC && hwnd) { ReleaseDC(hwnd, frontDC); frontDC = nullptr; }
    if (hwnd) { DestroyWindow(hwnd); hwnd = nullptr; }
}

// ========== 重置 ==========

void Game::resetGame() {
    delete player; player = nullptr;
    for (auto* e : enemies) delete e;
    for (auto* b : playerBullets) delete b;
    for (auto* b : enemyBullets) delete b;
    for (auto* i : items) delete i;
    enemies.clear(); playerBullets.clear(); enemyBullets.clear(); items.clear();
    particles.clear();

    player = new Player();
    score = 0; currentLevel = 1;
    scoreToNextLevel = 500; spawnInterval = 40;
    fastEnemyChance = 0.1f; shootingEnemyChance = 0;
    enemySpawnTimer = 0; enemiesKilledThisLevel = 0;
    bossSpawned = false; bossDefeated = false;
    shakeTimer = 0; shakeIntensity = 0;
    state = GameState::PLAYING; stateTimer = 0; frameCount = 0;
    QueryPerformanceCounter(&qpcPrev);
}

// ========== QPC 主循环 ==========

void Game::run() {
    MSG msg;
    while (true) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) { hwnd = nullptr; return; }
            TranslateMessage(&msg); DispatchMessage(&msg);
        }
        if (!hwnd) break;

        LARGE_INTEGER frameStart;
        QueryPerformanceCounter(&frameStart);
        deltaTime = (double)(frameStart.QuadPart - qpcPrev.QuadPart) / qpcFreq.QuadPart;
        qpcPrev = frameStart;

        handleInput();
        update();
        collide();
        render();

        // 精确帧率对齐 (60fps)
        LARGE_INTEGER now;
        do { QueryPerformanceCounter(&now); }
        while ((double)(now.QuadPart - frameStart.QuadPart) / qpcFreq.QuadPart < TARGET_FRAME_TIME);

        frameCount++;
    }
}

// ========== 输入 ==========

void Game::handleInput() {
    keyLeft  = (GetAsyncKeyState(VK_LEFT)  & 0x8000) || (GetAsyncKeyState('A') & 0x8000);
    keyRight = (GetAsyncKeyState(VK_RIGHT) & 0x8000) || (GetAsyncKeyState('D') & 0x8000);
    keyUp    = (GetAsyncKeyState(VK_UP)    & 0x8000) || (GetAsyncKeyState('W') & 0x8000);
    keyDown  = (GetAsyncKeyState(VK_DOWN)  & 0x8000) || (GetAsyncKeyState('S') & 0x8000);
    keySpace = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
    keyBomb  = (GetAsyncKeyState('B')      & 0x8000) != 0;
    keyPause = (GetAsyncKeyState('P')      & 0x8000) != 0;
    keyShift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    keyCtrl  = (GetAsyncKeyState(VK_CONTROL)&0x8000) != 0;

    static bool prevSpace = false;
    spacePressed = keySpace && !prevSpace; prevSpace = keySpace;

    static bool prevPause = false;
    if (keyPause && !prevPause) {
        if (state == GameState::PLAYING) state = GameState::PAUSED;
        else if (state == GameState::PAUSED) state = GameState::PLAYING;
    }
    prevPause = keyPause;

    static bool prevBomb = false;
    if (keyBomb && !prevBomb && state == GameState::PLAYING && player && player->bombCount > 0)
        activateBomb();
    prevBomb = keyBomb;

    if (menuCooldown > 0) menuCooldown--;

    // ── 菜单 ──
    if (state == GameState::MENU && menuCooldown <= 0) {
        bool up = (GetAsyncKeyState(VK_UP)&0x8000)||(GetAsyncKeyState('W')&0x8000);
        bool dn = (GetAsyncKeyState(VK_DOWN)&0x8000)||(GetAsyncKeyState('S')&0x8000);
        bool enter = (GetAsyncKeyState(VK_RETURN)&0x8000)!=0;
        int mc = 4;
        if (up) { menuSelection = (MenuOption)(((int)menuSelection-1+mc)%mc); menuCooldown=8; }
        if (dn) { menuSelection = (MenuOption)(((int)menuSelection+1)%mc); menuCooldown=8; }
        if (enter) {
            menuCooldown=15;
            switch (menuSelection) {
            case MenuOption::START_GAME: resetGame(); break;
            case MenuOption::LEADERBOARD: state=GameState::LEADERBOARD; break;
            case MenuOption::SETTINGS_MENU: settingsSelection=currentResolution; state=GameState::SETTINGS; break;
            case MenuOption::EXIT: PostQuitMessage(0); hwnd=nullptr; break;
            }
        }
    }

    // ── 设置 ──
    if (state == GameState::SETTINGS && menuCooldown <= 0) {
        bool up = (GetAsyncKeyState(VK_UP)&0x8000)||(GetAsyncKeyState('W')&0x8000);
        bool dn = (GetAsyncKeyState(VK_DOWN)&0x8000)||(GetAsyncKeyState('S')&0x8000);
        bool enter = (GetAsyncKeyState(VK_RETURN)&0x8000)!=0;
        bool esc = (GetAsyncKeyState(VK_ESCAPE)&0x8000)!=0;
        if (up) { settingsSelection=(settingsSelection-1+RESOLUTION_COUNT)%RESOLUTION_COUNT; menuCooldown=8; }
        if (dn) { settingsSelection=(settingsSelection+1)%RESOLUTION_COUNT; menuCooldown=8; }
        if (enter) { applyResolution(settingsSelection); state=GameState::MENU; menuCooldown=15; }
        if (esc) { state=GameState::MENU; menuCooldown=15; }
    }

    // ── 名字输入 ──
    if (state == GameState::NAME_ENTRY) {
        static bool prevEnterN = false;
        for (int k='A'; k<='Z'; k++) {
            static bool p[26]={};
            bool pr = (GetAsyncKeyState(k)&0x8000)!=0;
            if (pr && !p[k-'A'] && nameLength<3) nameBuffer[nameLength++]=(char)k;
            p[k-'A']=pr;
        }
        static bool prevBk=false;
        bool bk=(GetAsyncKeyState(VK_BACK)&0x8000)!=0;
        if (bk&&!prevBk&&nameLength>0) nameBuffer[--nameLength]='\0';
        prevBk=bk;
        bool en=(GetAsyncKeyState(VK_RETURN)&0x8000)!=0;
        if (en&&!prevEnterN&&nameLength>0) { nameBuffer[nameLength]='\0'; addScoreEntry(nameBuffer); state=GameState::LEADERBOARD; }
        prevEnterN=en;
        if (GetAsyncKeyState(VK_ESCAPE)&0x8000) state=GameState::MENU;
    }

    // ── 结束/胜利 ──
    if ((state==GameState::GAME_OVER||state==GameState::VICTORY) && (GetAsyncKeyState(VK_RETURN)&0x8000)) {
        if (isHighScore()) { nameLength=0; memset(nameBuffer,0,sizeof(nameBuffer)); state=GameState::NAME_ENTRY; }
        else state=GameState::MENU;
    }
    if (state==GameState::LEADERBOARD && (GetAsyncKeyState(VK_ESCAPE)&0x8000)) state=GameState::MENU;
}

// ========== 更新 ==========

void Game::update() {
    if (state != GameState::PLAYING && state != GameState::LEVEL_TRANSITION) return;

    if (shakeTimer > 0) shakeTimer--;

    // 星空
    for (auto& s : stars) { s.y += s.speed; if (s.y > WINDOW_HEIGHT+2) { s.y=-2; s.x=(float)(rand()%WINDOW_WIDTH); } }

    // 关卡过渡
    if (state == GameState::LEVEL_TRANSITION) {
        if (--stateTimer <= 0) state = GameState::PLAYING;
    }
    if (state != GameState::PLAYING) return;

    // ── 玩家 ──
    if (player) {
        // 变速模式
        player->setFastMode(keyShift);
        player->setSlowMode(keyCtrl);
        // 移动方向
        float spd = PLAYER_BASE_SPEED * GAME_SCALE;
        player->speedX = 0; player->speedY = 0;
        if (keyLeft)  player->speedX = -spd;
        if (keyRight) player->speedX =  spd;
        if (keyUp)    player->speedY = -spd;
        if (keyDown)  player->speedY =  spd;
        player->update();
        // ── 自动连续射击 ──
        if (player->canShoot()) {
            player->resetShootCooldown();
            float sc = GAME_SCALE;
            float cx = player->centerX(), by = player->y;
            int lv = player->firepowerLevel;
            if (lv>=1) playerBullets.push_back(new PlayerBullet(cx-BULLET_PLAYER_WIDTH*sc/2.0f, by));
            if (lv>=2) { playerBullets.push_back(new PlayerBullet(cx-BULLET_PLAYER_WIDTH*sc/2.0f-8*sc,by+4*sc)); playerBullets.push_back(new PlayerBullet(cx-BULLET_PLAYER_WIDTH*sc/2.0f+8*sc,by+4*sc)); }
            if (lv>=3) { playerBullets.push_back(new PlayerBullet(cx-BULLET_PLAYER_WIDTH*sc/2.0f-14*sc,by+8*sc)); playerBullets.push_back(new PlayerBullet(cx-BULLET_PLAYER_WIDTH*sc/2.0f+14*sc,by+8*sc)); }
            if (lv>=4) { playerBullets.push_back(new PlayerBullet(cx-BULLET_PLAYER_WIDTH*sc/2.0f-4*sc,by-4*sc)); playerBullets.push_back(new PlayerBullet(cx-BULLET_PLAYER_WIDTH*sc/2.0f+4*sc,by-4*sc)); }
            if (lv>=5) { playerBullets.push_back(new PlayerBullet(cx-BULLET_PLAYER_WIDTH*sc/2.0f-10*sc,by-4*sc)); playerBullets.push_back(new PlayerBullet(cx-BULLET_PLAYER_WIDTH*sc/2.0f+10*sc,by-4*sc)); }
        }
        if (score >= scoreToNextLevel && !bossSpawned && state==GameState::PLAYING && enemySpawnTimer<=0)
            advanceLevel();
    }

    spawnEnemies();

    for (auto* e : enemies) {
        // 追踪移动：朝玩家 X 微调
        if (e->movePattern==MovePattern::TRACKING && player) {
            float dx=player->centerX()-e->centerX();
            e->x += (dx>0?1.0f:-1.0f)*1.2f;  // 缓慢追踪玩家X
        }
        e->update();
        if (e->getType()==EnemyType::SHOOTING && e->shouldShoot(frameCount) && player) {
            e->resetShootTimer();
            float dx=player->centerX()-e->centerX(), dy=player->centerY()-e->centerY();
            float len=sqrtf(dx*dx+dy*dy);
            float ax=len>0?(dx/len)*BULLET_ENEMY_SPEED*0.4f:0;
            enemyBullets.push_back(new EnemyBullet(e->centerX()-3, e->y+e->height, ax));
            // Lv3+ 射击敌机 3 连发
            if (currentLevel>=3 && e->shootInterval<=30) {
                enemyBullets.push_back(new EnemyBullet(e->centerX()-3-6, e->y+e->height+4, ax-0.1f));
                enemyBullets.push_back(new EnemyBullet(e->centerX()-3+6, e->y+e->height+4, ax+0.1f));
            }
        }
        if (e->getType()==EnemyType::BOSS)
            static_cast<Boss*>(e)->firePattern(this);
    }

    for (auto* b : playerBullets) b->update();
    for (auto* b : enemyBullets)  b->update();
    for (auto* it: items)         it->update();

    // 粒子物理
    for (auto& p : particles) {
        if (!p.alive) continue;
        p.vx *= PARTICLE_DRAG; p.vy *= PARTICLE_DRAG;
        p.vy += PARTICLE_GRAVITY;
        p.x += p.vx; p.y += p.vy;
        if (--p.lifetime <= 0) p.alive = false;
    }

    outOfBoundsCleanup();

    // Boss 被击败推进
    if (bossDefeated && state==GameState::PLAYING) {
        bool st = false;
        for (auto* e : enemies) if (e->getType()==EnemyType::BOSS) { st=true; break; }
        if (!st) { bossDefeated=false; if (state!=GameState::VICTORY) startNextLevel(); }
    }
}

void Game::spawnEnemies() {
    if (enemySpawnTimer > 0) { enemySpawnTimer--; return; }
    if ((int)enemies.size() >= ENEMY_MAX_COUNT) return;
    bool ba=false; for (auto* e:enemies) if(e->getType()==EnemyType::BOSS&&e->alive){ba=true;break;}
    if (ba) return;
    if (bossSpawned && !ba) bossSpawned=false;

    enemySpawnTimer = (int)spawnInterval + rand()%10;
    int ew = (int)(ENEMY_NORMAL_WIDTH * GAME_SCALE);
    float sx = (float)(rand()%(WINDOW_WIDTH - ew));
    float roll = (rand()%100)/100.0f;

    if (roll < shootingEnemyChance) {
        auto* se = new ShootingEnemy(sx);
        // Lv3+ 射击敌机有概率带弹幕模式
        if (currentLevel>=3 && rand()%100<40) se->shootInterval = 25; // 快速连射
        enemies.push_back(se);
    }
    else if (roll < shootingEnemyChance+fastEnemyChance) {
        auto* fe = new FastEnemy(sx);
        // Lv3+ 快速敌机加入追踪
        if (currentLevel>=3 && rand()%100<50) { fe->movePattern=MovePattern::TRACKING; }
        // Lv4+ 可能有 Z 字路径
        if (currentLevel>=4 && rand()%100<30) { fe->movePattern=MovePattern::ZIGZAG; fe->sineAmplitude=4.0f; }
        enemies.push_back(fe);
    }
    else {
        auto* ne = new NormalEnemy(sx);
        if (currentLevel>=2) {
            int r = rand()%100;
            if (r < 25) { ne->movePattern=MovePattern::SINE; ne->sineAmplitude=2.0f+(rand()%3); ne->sinePhase=(rand()%628)/100.0f; }
            else if (r < 40 && currentLevel>=3) { ne->movePattern=MovePattern::ZIGZAG; ne->sineAmplitude=3.0f+(rand()%2); }
            else if (r < 50 && currentLevel>=4) { ne->movePattern=MovePattern::SPIRAL; ne->sinePhase=(rand()%628)/100.0f; }
        }
        enemies.push_back(ne);
    }
}

void Game::outOfBoundsCleanup() {
    for (auto it=enemies.begin(); it!=enemies.end();) {
        if (!(*it)->alive||(*it)->y>WINDOW_HEIGHT+50) { delete *it; it=enemies.erase(it); } else ++it;
    }
    for (auto it=playerBullets.begin(); it!=playerBullets.end();) {
        if ((*it)->y<-20||(*it)->y>WINDOW_HEIGHT+20) { delete *it; it=playerBullets.erase(it); } else ++it;
    }
    for (auto it=enemyBullets.begin(); it!=enemyBullets.end();) {
        if ((*it)->y<-20||(*it)->y>WINDOW_HEIGHT+20) { delete *it; it=enemyBullets.erase(it); } else ++it;
    }
    for (auto it=items.begin(); it!=items.end();) {
        if (!(*it)->alive||(*it)->y>WINDOW_HEIGHT+20) { delete *it; it=items.erase(it); } else ++it;
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(), [](const Particle& p){return !p.alive;}), particles.end());
}

// ========== 炸弹 ==========

void Game::activateBomb() {
    if (!player||player->bombCount<=0) return;
    player->bombCount--;
    for (auto* e : enemies) {
        if (e->getType()==EnemyType::BOSS&&e->alive) { bossDefeated=true; if (currentLevel>=LEVEL_COUNT) state=GameState::VICTORY; }
        spawnParticles(e->centerX(), e->centerY(), EXPLOSION_PARTICLES, Palette::ParticleExplosion, Palette::ParticleExplosion, 0.8f,4,1,3);
        if (e->getType()!=EnemyType::BOSS) score+=e->scoreValue;
        delete e;
    }
    enemies.clear();
    for (auto* b:enemyBullets) delete b;
    enemyBullets.clear();
    shakeTimer=20; shakeIntensity=6;

    for (int i=0;i<80;i++) {
        Particle p={};
        p.x=(float)(rand()%WINDOW_WIDTH); p.y=(float)(rand()%WINDOW_HEIGHT);
        p.vx=((rand()%100)/50.0f-1)*0.5f; p.vy=((rand()%100)/50.0f-1)*0.5f;
        p.lifetime=15+rand()%20; p.maxLifetime=p.lifetime;
        p.color=Palette::ParticleBomb; p.trailColor=Color(255,255,255,180); p.size=1.5f; p.alive=true;
        particles.push_back(p);
    }
    for (int ring=0;ring<3;ring++)
        for (int i=0;i<30;i++) {
            Particle p={};
            float ang=i*3.14159f*2/30, dist=10+ring*30;
            p.x=WINDOW_WIDTH/2.0f+cosf(ang)*dist; p.y=WINDOW_HEIGHT/2.0f+sinf(ang)*dist;
            p.vx=cosf(ang)*(3+ring*1.5f); p.vy=sinf(ang)*(3+ring*1.5f);
            p.lifetime=20+ring*5; p.maxLifetime=p.lifetime;
            p.color=Palette::ParticleBomb; p.trailColor=Color(255,255,255,200); p.size=1.5f; p.alive=true;
            particles.push_back(p);
        }
}

// ========== 碰撞 ==========

void Game::collide() {
    if (state!=GameState::PLAYING||!player) return;
    checkBulletEnemyCollisions();
    checkEnemyBulletPlayerCollisions();
    checkEnemyPlayerCollisions();
    checkItemPlayerCollisions();
}

void Game::checkBulletEnemyCollisions() {
    for (auto* b : playerBullets) {
        if (!b->alive) continue;
        for (auto* e : enemies) {
            if (!e->alive) continue;
            if (checkCollision(*b, *e)) {
                b->alive=false; e->hp-=b->damage;
                if (e->hp<=0) {
                    e->alive=false; score+=e->scoreValue; enemiesKilledThisLevel++;
                    if (e->getType()==EnemyType::BOSS) {
                        for (int w=0;w<BOSS_EXPLOSION_WAVES;w++)
                            spawnParticles(e->centerX(),e->centerY(),EXPLOSION_PARTICLES*4,
                                Color(255,255,40+w*60,0),Color(255,255,180,20+w*30),1+w*0.5f,5+w*1.5f,1.5f,4);
                        for (int d=0;d<3;d++) {
                            float rr=(rand()%100)/100.0f; ItemType it;
                            if (rr<0.35f) it=ItemType::FIREPOWER; else if(rr<0.55f) it=ItemType::HEALTH;
                            else if(rr<0.70f) it=ItemType::BOMB; else if(rr<0.85f) it=ItemType::SHIELD;
                            else it=ItemType::FIRERATE;
                            items.push_back(new Item(e->centerX()-ITEM_SIZE/2.0f+(d-1)*30.0f, e->y, it));
                        }
                        shakeTimer=30; shakeIntensity=8; bossDefeated=true;
                        if (currentLevel>=LEVEL_COUNT) state=GameState::VICTORY;
                    } else {
                        spawnParticles(e->centerX(),e->centerY(),EXPLOSION_PARTICLES,
                            Palette::ParticleExplosion,Palette::ParticleExplosion,0.5f,3.5f,1,2.5f);
                        if ((rand()%100)/100.0f<ITEM_DROP_CHANCE) {
                            float rr=(rand()%100)/100.0f; ItemType it;
                            if (rr<0.35f) it=ItemType::FIREPOWER; else if(rr<0.55f) it=ItemType::HEALTH;
                            else if(rr<0.70f) it=ItemType::BOMB; else if(rr<0.85f) it=ItemType::SHIELD;
                            else it=ItemType::FIRERATE;
                            items.push_back(new Item(e->centerX()-ITEM_SIZE/2.0f, e->y, it));
                        }
                    }
                }
                break;
            }
        }
    }
}

void Game::checkEnemyBulletPlayerCollisions() {
    for (auto* b:enemyBullets) {
        if (!b->alive) continue;
        if (checkCollision(*b,*player)) {
            b->alive=false; player->takeDamage();
            spawnParticles(player->centerX(),player->centerY(),15,Color(255,255,200,60),Color(255,255,150,30),0.5f,2.5f,0.5f,1.5f);
            shakeTimer=10; shakeIntensity=4;
            if (player->lives<=0) state=GameState::GAME_OVER;
            return;
        }
    }
}

void Game::checkEnemyPlayerCollisions() {
    if (player->isInvincible()) return;
    for (auto* e:enemies) {
        if (!e->alive) continue;
        if (checkCollision(*player,*e)) {
            e->alive=false; player->takeDamage();
            spawnParticles(e->centerX(),e->centerY(),EXPLOSION_PARTICLES/2,Palette::ParticleExplosion,Palette::ParticleExplosion,0.5f,2.5f,1,2);
            spawnParticles(player->centerX(),player->centerY(),12,Color(255,255,200,60),Color(255,255,150,30),0.5f,2,0.5f,1.5f);
            shakeTimer=15; shakeIntensity=5;
            if (player->lives<=0) state=GameState::GAME_OVER;
            return;
        }
    }
}

void Game::checkItemPlayerCollisions() {
    for (auto* it:items) {
        if (!it->alive) continue;
        if (checkCollision(*player,*it)) {
            it->alive=false;
            switch (it->itemType) {
            case ItemType::FIREPOWER: player->increaseFirepower(); break;
            case ItemType::HEALTH: player->addLife(); break;
            case ItemType::BOMB: player->addBomb(); break;
            case ItemType::SHIELD: player->activateShield(); break;
            case ItemType::FIRERATE: player->boostFireRate(); break;
            }
            for (int i=0;i<10;i++) {
                Particle p={};
                p.x=it->centerX(); p.y=it->centerY();
                float ang=i*3.14159f*2/10;
                p.vx=cosf(ang)*2.5f; p.vy=sinf(ang)*2.5f;
                p.lifetime=18; p.maxLifetime=18;
                p.color=Color(255,255,255,130); p.trailColor=Color(255,200,200,80); p.size=1; p.alive=true;
                particles.push_back(p);
            }
        }
    }
}

// ========== 粒子 ==========

void Game::spawnParticles(float cx, float cy, int cnt, Color c, Color tc, float minSpd, float maxSpd, float minSz, float maxSz) {
    for (int i=0; i<cnt && (int)particles.size()<PARTICLE_MAX; i++) {
        Particle p={};
        p.x=cx; p.y=cy;
        float ang=(rand()%628)/100.0f, spd=minSpd+(rand()%100)/100.0f*(maxSpd-minSpd);
        p.vx=cosf(ang)*spd; p.vy=sinf(ang)*spd-(rand()%100)/100.0f;
        p.lifetime=15+rand()%EXPLOSION_LIFE; p.maxLifetime=p.lifetime;
        p.color=c; p.trailColor=tc;
        p.size=minSz+(rand()%100)/100.0f*(maxSz-minSz); p.alive=true;
        particles.push_back(p);
    }
}

// ========== 关卡 ==========

void Game::advanceLevel() {
    bossSpawned=true;
    for (auto* e:enemies) {
        if (e->getType()!=EnemyType::BOSS) { spawnParticles(e->centerX(),e->centerY(),6,Color(255,255,200,60),Color(255,255,150,30),0.3f,1.5f,1,1.5f); delete e; }
    }
    enemies.erase(std::remove_if(enemies.begin(),enemies.end(),[](Enemy*e){return e->getType()!=EnemyType::BOSS;}), enemies.end());
    for (auto* b:enemyBullets) delete b;
    enemyBullets.clear();
    enemies.push_back(new Boss(WINDOW_WIDTH/2.0f-BOSS_WIDTH/2.0f, currentLevel));
}

void Game::startNextLevel() {
    currentLevel++; state=GameState::LEVEL_TRANSITION; stateTimer=LEVEL_TRANSITION_FRAMES; enemySpawnTimer=60;
    switch (currentLevel) {
    case 1: scoreToNextLevel=score+500; spawnInterval=40; fastEnemyChance=0.1f; shootingEnemyChance=0; break;
    case 2: scoreToNextLevel=score+800; spawnInterval=28; fastEnemyChance=0.25f; shootingEnemyChance=0.1f; break;
    case 3: scoreToNextLevel=score+1200; spawnInterval=20; fastEnemyChance=0.3f; shootingEnemyChance=0.2f; break;
    case 4: scoreToNextLevel=score+1600; spawnInterval=15; fastEnemyChance=0.35f; shootingEnemyChance=0.3f; break;
    default: scoreToNextLevel=score+2000; spawnInterval=std::max(8.0f,spawnInterval-3); fastEnemyChance=std::min(0.5f,fastEnemyChance+0.05f); shootingEnemyChance=std::min(0.4f,shootingEnemyChance+0.05f); break;
    }
    bossSpawned=false; bossDefeated=false;
}

// ========== 排行榜 ==========

void Game::loadLeaderboard() {
    leaderboard.clear();
    char path[MAX_PATH]; GetModuleFileNameA(nullptr,path,MAX_PATH);
    char* sl=strrchr(path,'\\'); if(sl) *(sl+1)='\0';
    strcat_s(path, sizeof(path), "scores.dat");
    HANDLE h=CreateFileA(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if (h==INVALID_HANDLE_VALUE) return;
    DWORD cnt=0,br;
    if (!ReadFile(h,&cnt,sizeof(DWORD),&br,nullptr)||cnt>50) { CloseHandle(h); return; }
    for (DWORD i=0;i<cnt;i++) { ScoreEntry e; memset(&e,0,sizeof(e)); if(!ReadFile(h,&e,sizeof(ScoreEntry),&br,nullptr)) break; leaderboard.push_back(e); }
    CloseHandle(h);
    if (!leaderboard.empty()) highScore=leaderboard[0].score;
}

void Game::saveLeaderboard() {
    char path[MAX_PATH]; GetModuleFileNameA(nullptr,path,MAX_PATH);
    char* sl=strrchr(path,'\\'); if(sl) *(sl+1)='\0';
    strcat_s(path, sizeof(path), "scores.dat");
    HANDLE h=CreateFileA(path,GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if (h==INVALID_HANDLE_VALUE) return;
    DWORD cnt=(DWORD)leaderboard.size(),bw;
    WriteFile(h,&cnt,sizeof(DWORD),&bw,nullptr);
    for (auto& e:leaderboard) WriteFile(h,&e,sizeof(ScoreEntry),&bw,nullptr);
    CloseHandle(h);
}

void Game::addScoreEntry(const char* name) {
    char d[11]; SYSTEMTIME st; GetLocalTime(&st); sprintf_s(d, sizeof(d), "%04d-%02d-%02d", (int)st.wYear, (int)st.wMonth, (int)st.wDay);
    ScoreEntry e; memset(&e,0,sizeof(e)); strncpy_s(e.name, sizeof(e.name), name, NAME_MAX_LEN-1); e.score=score; e.level=currentLevel; strncpy_s(e.date, sizeof(e.date), d, 10);
    auto it=leaderboard.begin(); while(it!=leaderboard.end()&&it->score>score) ++it;
    leaderboard.insert(it,e);
    if (leaderboard.size()>LEADERBOARD_MAX) leaderboard.pop_back();
    if (!leaderboard.empty()) highScore=leaderboard[0].score;
    saveLeaderboard();
}

bool Game::isHighScore() const { return leaderboard.size()<LEADERBOARD_MAX || score>leaderboard.back().score; }

// ========================================================================
//  渲染 — 全部使用 GDI+
// ========================================================================

void Game::render() {
    // 双缓冲 — 创建 GDI+ Graphics
    HDC memDC = CreateCompatibleDC(frontDC);
    HBITMAP memBmp = CreateCompatibleBitmap(frontDC, WINDOW_WIDTH, WINDOW_HEIGHT);
    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

    Graphics g(memDC);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintAntiAlias);

    // 清屏
    g.Clear(Color(255, 2, 2, 18));

    switch (state) {
    case GameState::MENU:       renderMenu(g); break;
    case GameState::SETTINGS:   renderSettings(g); break;
    case GameState::PLAYING:
    case GameState::PAUSED:
    case GameState::LEVEL_TRANSITION:
        renderBackground(g);
        renderParticles(g);
        renderItems(g);
        renderEnemies(g);
        // Boss HP bar
        for (auto* e : enemies)
            if (e->getType()==EnemyType::BOSS && e->alive)
                static_cast<Boss*>(e)->renderHPBar(g);
        renderBullets(g);
        if (player) player->render(g);
        renderHUD(g);
        if (state==GameState::PAUSED) renderPauseOverlay(g);
        if (state==GameState::LEVEL_TRANSITION) renderLevelTransition(g);
        break;
    case GameState::HELP:       renderBackground(g); renderParticles(g); renderItems(g); renderEnemies(g); renderBullets(g); if(player) player->render(g); renderHUD(g); renderHelp(g); break;
    case GameState::GAME_OVER:
    case GameState::VICTORY:    renderGameOver(g); break;
    case GameState::LEADERBOARD: renderLeaderboard(g); break;
    case GameState::NAME_ENTRY:  renderNameEntry(g); break;
    }

    // Flush & Blt
    g.Flush();
    BitBlt(frontDC, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldBmp);
    DeleteObject(memBmp);
    DeleteDC(memDC);
}

// ========== 平铺背景（带亮度控制） ==========

void Game::renderTiledBG(Graphics& g, Image* img) {
    if (!img) return;
    int iw = img->GetWidth(), ih = img->GetHeight();
    if (iw<=0||ih<=0) return;
    for (int y=0; y<WINDOW_HEIGHT; y+=ih)
        for (int x=0; x<WINDOW_WIDTH; x+=iw)
            g.DrawImage(img, x, y, iw, ih);
    // 亮度控制：暗色叠加层
    if (bgAlpha < 255) {
        int a = 255 - bgAlpha;
        if (a > 0) {
            SolidBrush dim(Color((BYTE)a, 0, 0, 0));
            g.FillRectangle(&dim, 0.0f, 0.0f, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT);
        }
    }
}

void Game::renderBackground(Graphics& g) {
    renderTiledBG(g, bgImage);
    // 星空叠加
    for (auto& s : stars) {
        s.twinkleTimer++;
        int br = s.brightness;
        if (s.size>=2) { float tw=sinf((s.twinkleTimer+s.twinklePhase)*0.05f); br=(int)(br*(0.6f+0.4f*tw)); }
        br=(std::max)(20,(std::min)(255,br));
        Color c(s.color.GetA(), (BYTE)(s.color.GetR()*br/255), (BYTE)(s.color.GetG()*br/255), (BYTE)(s.color.GetB()*br/255));
        SolidBrush b(c);
        if (s.size>=2) { g.FillRectangle(&b, (int)s.x, (int)s.y, 2, 2);
            if (s.size>=3&&br>180) { g.FillRectangle(&b, (int)s.x+2, (int)s.y, 1, 1); g.FillRectangle(&b, (int)s.x-2, (int)s.y, 1, 1); g.FillRectangle(&b, (int)s.x, (int)s.y+2, 1, 1); g.FillRectangle(&b, (int)s.x, (int)s.y-2, 1, 1); }
        } else { g.FillRectangle(&b, (int)s.x, (int)s.y, 1, 1); }
    }
}

// ========== 粒子 ==========

void Game::renderParticles(Graphics& g) {
    for (auto& p : particles) {
        if (!p.alive) continue;
        float alpha = (float)p.lifetime/p.maxLifetime;
        // 拖尾
        Color tc(p.trailColor.GetA(), (BYTE)(p.trailColor.GetR()*alpha*0.4f), (BYTE)(p.trailColor.GetG()*alpha*0.4f), (BYTE)(p.trailColor.GetB()*alpha*0.4f));
        SolidBrush tb(tc);
        g.FillRectangle(&tb, p.x-p.vx, p.y-p.vy, (std::max)(1.0f,p.size*0.7f), (std::max)(1.0f,p.size*0.7f));
        // 主体
        Color mc(p.color.GetA(), (BYTE)(p.color.GetR()*alpha), (BYTE)(p.color.GetG()*alpha), (BYTE)(p.color.GetB()*alpha));
        SolidBrush mb(mc);
        g.FillRectangle(&mb, p.x, p.y, (std::max)(1.0f,p.size), (std::max)(1.0f,p.size));
    }
}

// ========== HUD ==========

void Game::renderHUD(Graphics& g) {
    if (!player) return;
    float sc = (std::max)(0.8f, WINDOW_WIDTH/640.0f); // UI 缩放
    int fs=(int)(12*sc), fsSm=(int)(9*sc);
    FontFamily ff(L"Consolas"); if(!ff.IsAvailable()) FontFamily(L"Arial");
    Font fnt(&ff, (float)fs, FontStyleBold, UnitPixel);
    Font fntSm(&ff, (float)fsSm, FontStyleRegular, UnitPixel);
    SolidBrush wh(Color::White), red(Color(255,255,60,60)), orn(Color(255,255,165,20)), yel(Color(255,255,225,40)), blu(Color(255,0,195,255)), gry(Color(255,200,200,200)), dim(Color(255,140,140,140)), wrn(Color(255,255,45,45));

    char buf[128]; float yPos=4.0f;
    sprintf_s(buf, sizeof(buf), "SCORE %d",score); g.DrawString(widen(buf),-1,&fnt,PointF(6,yPos),&wh);
    yPos+=16*sc;
    sprintf_s(buf, sizeof(buf), "LIFE"); g.DrawString(widen(buf),-1,&fnt,PointF(6,yPos),&red);
    for (int i=0;i<player->lives;i++) { sprintf_s(buf, sizeof(buf), "O"); g.DrawString(widen(buf),-1,&fnt,PointF(48+i*16.0f*sc,yPos),&red); }
    yPos+=16*sc;
    sprintf_s(buf, sizeof(buf), "BOMB %d",player->bombCount); g.DrawString(widen(buf),-1,&fnt,PointF(6,yPos),&orn);
    yPos+=16*sc;
    sprintf_s(buf, sizeof(buf), "PWR Lv.%d",player->firepowerLevel); g.DrawString(widen(buf),-1,&fnt,PointF(6,yPos),&yel);
    if (player->shieldActive) {
        yPos+=16*sc;
        sprintf_s(buf, sizeof(buf), "SHD %ds",player->shieldTimer/60); g.DrawString(widen(buf),-1,&fnt,PointF(6,yPos),&blu);
    }
    if (player->fireRateBoost) {
        yPos+=16*sc;
        SolidBrush frBr(Color(255,0,255,120));
        sprintf_s(buf, sizeof(buf), "RAPID %ds",player->fireRateTimer/60); g.DrawString(widen(buf),-1,&fnt,PointF(6,yPos),&frBr);
    }
    // 右上
    float rx=WINDOW_WIDTH-120.0f;
    sprintf_s(buf, sizeof(buf), "LV %d",currentLevel); g.DrawString(widen(buf),-1,&fnt,PointF(rx,4),&gry);
    if (!bossSpawned) { sprintf_s(buf, sizeof(buf), "NXT %d",scoreToNextLevel); g.DrawString(widen(buf),-1,&fnt,PointF(rx,18*sc+4),&gry); }
    else { g.DrawString(widen("[BOSS]"),-1,&fnt,PointF(rx,18*sc+4),&wrn); }
    sprintf_s(buf, sizeof(buf), "HST %d",(int)enemies.size()); g.DrawString(widen(buf),-1,&fnt,PointF(rx,36*sc+4),&dim);
    sprintf_s(buf, sizeof(buf), "BST %d",highScore); g.DrawString(widen(buf),-1,&fnt,PointF(rx,54*sc+4),&dim);

    // 速度指示
    if (player && (keyShift || keyCtrl)) {
        SolidBrush spdBr(keyShift ? Color(255,0,255,100) : Color(255,100,200,255));
        g.DrawString(widen(keyShift?"SPEED":"SLOW"),-1,&fnt,PointF(WINDOW_WIDTH/2.0f-25,WINDOW_HEIGHT-20.0f),&spdBr);
    }

    // ── 底部按键提示条 ──
    SolidBrush hintBr(Color(180,160,160,160));
    g.DrawString(widen("[H] Help  [Shift] Speed  [Ctrl] Slow  [B] Bomb  [P] Pause  AutoFire ON"), -1, &fntSm,
        PointF(6, WINDOW_HEIGHT-14.0f), &hintBr);
}

// ========== 菜单 ==========

void Game::renderMenu(Graphics& g) {
    float sc=(WINDOW_HEIGHT/480.0f);
    FontFamily ff(L"Arial");
    int tyOff = (int)(sinf(frameCount*0.03f)*3);

    Font tFnt(&ff, 42*sc, FontStyleBold, UnitPixel);
    SolidBrush gold(Color(255,255,220,0));
    RectF tRc(0, 35*sc+tyOff, (float)WINDOW_WIDTH, 115*sc+tyOff);
    StringFormat sf; sf.SetAlignment(StringAlignmentCenter); sf.SetLineAlignment(StringAlignmentCenter);
    g.DrawString(L"PLANE WAR", -1, &tFnt, tRc, &sf, &gold);

    Font sFnt(&ff, 12*sc, FontStyleRegular, UnitPixel);
    SolidBrush subC(Color(255,150,150,150));
    RectF sRc(0, 120*sc, (float)WINDOW_WIDTH, 140*sc);
    g.DrawString(L"GDI+ Edition | 60 FPS | H=Help", -1, &sFnt, sRc, &sf, &subC);

    Pen sep(Color(255,60,60,80), 1);
    float lineY=155*sc;
    g.DrawLine(&sep, WINDOW_WIDTH/2.0f-100*sc, lineY, WINDOW_WIDTH/2.0f+100*sc, lineY);

    Font mFnt(&ff, 22*sc, FontStyleRegular, UnitPixel);
    SolidBrush selC(Color(255,255,255,140)), norC(Color(255,185,185,185));
    const wchar_t* items[] = {L"START GAME", L"LEADERBOARD", L"SETTINGS", L"EXIT"};
    float optStart=170*sc, optStep=38*sc;
    for (int i=0;i<4;i++) {
        SolidBrush& br=(int)menuSelection==i?selC:norC;
        wchar_t buf[64];
        swprintf_s(buf,_countof(buf),(int)menuSelection==i?L">  %s":L"   %s",items[i]);
        RectF iRc(0,optStart+i*optStep,(float)WINDOW_WIDTH,optStart+(i+1)*optStep);
        g.DrawString(buf,-1,&mFnt,iRc,&sf,&br);
    }

    float hintY=optStart+4*optStep+8*sc;
    Font hFnt(&ff, 10*sc, FontStyleRegular, UnitPixel);
    SolidBrush hnt(Color(255,110,110,110));
    RectF hRc(0, hintY, (float)WINDOW_WIDTH, hintY+30*sc);
    g.DrawString(L"WASD Move  AutoFire  Shift Speed  Ctrl Slow  B Bomb  P Pause  H Help",-1,&hFnt,hRc,&sf,&hnt);
}

// ========== 设置 ==========

void Game::renderSettings(Graphics& g) {
    float sc=(WINDOW_HEIGHT/480.0f);
    FontFamily ff(L"Arial");
    Font tFnt(&ff, 26*sc, FontStyleBold, UnitPixel);
    Font oFnt(&ff, 16*sc, FontStyleRegular, UnitPixel);
    Font hFnt(&ff, 11*sc, FontStyleRegular, UnitPixel);
    SolidBrush gold(Color(255,255,220,0)), sel(Color(255,255,255,140)), nor(Color(255,185,185,185)), grn(Color(255,0,255,130)), dim(Color(255,140,140,140)), hnt(Color(255,110,110,110));
    StringFormat sf; sf.SetAlignment(StringAlignmentCenter); sf.SetLineAlignment(StringAlignmentCenter);

    RectF tRc(0,25*sc,(float)WINDOW_WIDTH,70*sc); g.DrawString(L"SETTINGS",-1,&tFnt,tRc,&sf,&gold);
    RectF sRc(0,70*sc,(float)WINDOW_WIDTH,95*sc); g.DrawString(L"Resolution",-1,&oFnt,sRc,&sf,&dim);

    StringFormat lf; lf.SetAlignment(StringAlignmentNear); lf.SetLineAlignment(StringAlignmentCenter);
    float optStart=110*sc, optStep=42*sc;
    for (int i=0;i<RESOLUTION_COUNT;i++) {
        SolidBrush& br=(i==settingsSelection&&i==currentResolution)?grn:(i==settingsSelection?sel:(i==currentResolution?grn:nor));
        wchar_t buf[80]; swprintf_s(buf,_countof(buf),L"%hs  %hs",RESOLUTIONS[i].label,i==currentResolution?L" [ACTIVE]":L"");
        RectF r(80*sc,optStart+i*optStep,WINDOW_WIDTH-80*sc,optStart+(i+1)*optStep);
        g.DrawString(buf,-1,&oFnt,r,&lf,&br);
    }
    float hintY=optStart+4*optStep+6*sc;
    RectF bRc(0,hintY,(float)WINDOW_WIDTH,hintY+30*sc);
    g.DrawString(L"Arrows=Select  Enter=Apply  ESC=Back",-1,&hFnt,bRc,&sf,&hnt);
}

// ========== 暂停 ==========

void Game::renderPauseOverlay(Graphics& g) {
    SolidBrush overlay(Color(160,0,0,0));
    g.FillRectangle(&overlay, 0.0f, 0.0f, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT);
    float sc=(WINDOW_HEIGHT/480.0f);
    FontFamily ff(L"Arial");
    Font tFnt(&ff, 30*sc, FontStyleBold, UnitPixel);
    Font sFnt(&ff, 15*sc, FontStyleRegular, UnitPixel);
    SolidBrush gold(Color(255,255,220,0)), nor(Color(255,185,185,185));
    StringFormat sf; sf.SetAlignment(StringAlignmentCenter); sf.SetLineAlignment(StringAlignmentCenter);
    RectF tRc(0,120*sc,(float)WINDOW_WIDTH,180*sc); g.DrawString(L"PAUSED",-1,&tFnt,tRc,&sf,&gold);
    RectF sRc(0,220*sc,(float)WINDOW_WIDTH,260*sc);
    g.DrawString(L"P = Resume    ESC = Menu",-1,&sFnt,sRc,&sf,&nor);
}

// ========== 结束/胜利 ==========

void Game::renderGameOver(Graphics& g) {
    SolidBrush overlay(Color(190,2,2,18)); g.FillRectangle(&overlay,0.0f,0.0f,(float)WINDOW_WIDTH,(float)WINDOW_HEIGHT);
    float sc=(WINDOW_HEIGHT/480.0f);
    bool vic=(state==GameState::VICTORY);
    FontFamily ff(L"Arial");
    Font tFnt(&ff, 34*sc, FontStyleBold, UnitPixel);
    Font sFnt(&ff, 17*sc, FontStyleRegular, UnitPixel);
    SolidBrush gold(Color(255,255,220,0)), red(Color(255,255,45,45)), wh(Color::White), nor(Color(255,185,185,185));
    StringFormat sf; sf.SetAlignment(StringAlignmentCenter); sf.SetLineAlignment(StringAlignmentCenter);

    RectF tRc(0,40*sc,(float)WINDOW_WIDTH,100*sc); g.DrawString(vic?L"VICTORY":L"GAME OVER",-1,&tFnt,tRc,&sf,vic?&gold:&red);
    wchar_t buf[128];
    float y=120*sc, step=30*sc;
    swprintf_s(buf,_countof(buf),L"FINAL SCORE: %d",score); RectF r2(0,y,(float)WINDOW_WIDTH,y+step); g.DrawString(buf,-1,&sFnt,r2,&sf,&wh);
    y+=step; swprintf_s(buf,_countof(buf),L"LEVEL: %d",currentLevel); RectF r3(0,y,(float)WINDOW_WIDTH,y+step); g.DrawString(buf,-1,&sFnt,r3,&sf,&nor);
    y+=step; swprintf_s(buf,_countof(buf),L"HOSTILES: %d",enemiesKilledThisLevel); RectF r4(0,y,(float)WINDOW_WIDTH,y+step); g.DrawString(buf,-1,&sFnt,r4,&sf,&nor);
    y+=step*1.5f;
    if (isHighScore()) {
        RectF r6(0,y,(float)WINDOW_WIDTH,y+step); g.DrawString(L"NEW HIGH SCORE",-1,&sFnt,r6,&sf,&gold);
        y+=step;
        RectF r7(0,y,(float)WINDOW_WIDTH,y+step); g.DrawString(L"Press Enter to Record",-1,&sFnt,r7,&sf,&gold);
        y+=step;
    }
    RectF r5(0,y,(float)WINDOW_WIDTH,y+step); g.DrawString(L"Press Enter to Continue",-1,&sFnt,r5,&sf,&nor);
}

// ========== 排行榜 ==========

void Game::renderLeaderboard(Graphics& g) {
    float sc=(WINDOW_HEIGHT/480.0f);
    FontFamily ff(L"Arial"), ffm(L"Consolas");
    Font tFnt(&ff, 26*sc, FontStyleBold, UnitPixel);
    Font lFnt(&ffm, 13*sc, FontStyleRegular, UnitPixel);
    SolidBrush gold(Color(255,255,220,0)), dim(Color(255,140,140,140)), hnt(Color(255,110,110,110));
    StringFormat sf,lf; sf.SetAlignment(StringAlignmentCenter); sf.SetLineAlignment(StringAlignmentCenter);
    lf.SetAlignment(StringAlignmentNear); lf.SetLineAlignment(StringAlignmentCenter);

    RectF tRc(0,8*sc,(float)WINDOW_WIDTH,42*sc); g.DrawString(L"LEADERBOARD",-1,&tFnt,tRc,&sf,&gold);
    wchar_t buf[128]; swprintf_s(buf,_countof(buf),L"RANK NAME      SCORE  LV DATE");
    RectF hRc(25,48*sc,WINDOW_WIDTH-25.0f,65*sc); g.DrawString(buf,-1,&lFnt,hRc,&lf,&dim);
    Pen sep(Color(255,70,70,70),1); g.DrawLine(&sep,25.0f,65*sc,WINDOW_WIDTH-25.0f,65*sc);

    float rowH=20*sc, startY=70*sc;
    for (size_t i=0;i<leaderboard.size();i++) {
        auto& e=leaderboard[i];
        SolidBrush br((i==0)?Color(255,255,215,0):(i==1)?Color(255,192,192,192):(i==2)?Color(255,205,127,50):Color(255,190,190,190));
        wchar_t n[32]={}; mbstowcs(n,e.name,NAME_MAX_LEN);
        swprintf_s(buf,_countof(buf),L"%-4d %-9s %-7d %-3d %hs",(int)(i+1),n,e.score,e.level,e.date);
        RectF iRc(25,startY+i*rowH,WINDOW_WIDTH-25.0f,startY+(i+1)*rowH); g.DrawString(buf,-1,&lFnt,iRc,&lf,&br);
    }
    if (leaderboard.empty()) { RectF eRc(0,80*sc,(float)WINDOW_WIDTH,120*sc); g.DrawString(L"No records. Go play!",-1,&lFnt,eRc,&sf,&dim); }
    RectF bRc(0,WINDOW_HEIGHT-26*sc,(float)WINDOW_WIDTH,WINDOW_HEIGHT-6*sc); g.DrawString(L"ESC = Back",-1,&lFnt,bRc,&sf,&hnt);
}

// ========== 名字输入 ==========

void Game::renderNameEntry(Graphics& g) {
    SolidBrush bg(Color(200,3,3,22)); g.FillRectangle(&bg,0.0f,0.0f,(float)WINDOW_WIDTH,(float)WINDOW_HEIGHT);
    float sc=(WINDOW_HEIGHT/480.0f);
    FontFamily ff(L"Arial"), ffm(L"Consolas");
    Font tFnt(&ff, 26*sc, FontStyleBold, UnitPixel);
    Font sFnt(&ffm, 15*sc, FontStyleRegular, UnitPixel);
    Font nFnt(&ffm, 32*sc, FontStyleBold, UnitPixel);
    SolidBrush gold(Color(255,255,220,0)), wh(Color::White), nor(Color(255,185,185,185)), cya(Color(255,0,255,255)), hnt(Color(255,110,110,110));
    StringFormat sf; sf.SetAlignment(StringAlignmentCenter); sf.SetLineAlignment(StringAlignmentCenter);

    RectF tRc(0,50*sc,(float)WINDOW_WIDTH,90*sc); g.DrawString(L"NEW HIGH SCORE",-1,&tFnt,tRc,&sf,&gold);
    wchar_t buf[128]; swprintf_s(buf,_countof(buf),L"Score: %d   Level: %d",score,currentLevel);
    RectF r2(0,110*sc,(float)WINDOW_WIDTH,140*sc); g.DrawString(buf,-1,&sFnt,r2,&sf,&wh);
    RectF r3(0,165*sc,(float)WINDOW_WIDTH,195*sc); g.DrawString(L"Enter name (3 letters):",-1,&sFnt,r3,&sf,&nor);
    wchar_t nd[32]={}; for(int i=0;i<nameLength;i++) nd[i]=(wchar_t)nameBuffer[i]; wcscat(nd,L"_");
    RectF r4(0,210*sc,(float)WINDOW_WIDTH,260*sc); g.DrawString(nd,-1,&nFnt,r4,&sf,&cya);
    RectF r5(0,300*sc,(float)WINDOW_WIDTH,335*sc);
    g.DrawString(L"A-Z=Input  Backspace=Del  Enter=OK  ESC=Skip",-1,&sFnt,r5,&sf,&hnt);
}

// ========== 关卡过渡 ==========

void Game::renderLevelTransition(Graphics& g) {
    float prog=1.0f-(float)stateTimer/LEVEL_TRANSITION_FRAMES;
    float sz=1.0f+sinf(prog*3.14159f)*0.3f;
    int fs=(int)(38*sz), al=(int)(255*(1.0f-fabsf(prog-0.5f)*2.0f));
    al=(std::max)(60,(std::min)(255,al));
    FontFamily ff(L"Arial");
    Font fnt(&ff, (float)fs, FontStyleBold, UnitPixel);
    SolidBrush br(Color((BYTE)al,(BYTE)(al*200/255),0));
    StringFormat sf; sf.SetAlignment(StringAlignmentCenter); sf.SetLineAlignment(StringAlignmentCenter);
    wchar_t buf[64]; swprintf_s(buf,_countof(buf),L"LEVEL %d",currentLevel);
    RectF rc(0,140,(float)WINDOW_WIDTH,240); g.DrawString(buf,-1,&fnt,rc,&sf,&br);
}

// ========== 帮助画面 ==========

void Game::renderHelp(Graphics& g) {
    SolidBrush overlay(Color(200,4,4,24));
    g.FillRectangle(&overlay, 0.0f, 0.0f, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT);
    float sc=(WINDOW_HEIGHT/480.0f);
    FontFamily ff(L"Arial"), ffm(L"Consolas");
    Font tFnt(&ff, 24*sc, FontStyleBold, UnitPixel), kFnt(&ffm, 12*sc, FontStyleBold, UnitPixel), dFnt(&ff, 11*sc, FontStyleRegular, UnitPixel);
    SolidBrush gold(Color(255,255,220,0)), wh(Color::White), dim(Color(255,180,180,180)), hnt(Color(255,130,130,130));
    StringFormat sf,lf; sf.SetAlignment(StringAlignmentCenter); sf.SetLineAlignment(StringAlignmentCenter);
    lf.SetAlignment(StringAlignmentNear); lf.SetLineAlignment(StringAlignmentCenter);

    RectF tRc(0,8*sc,(float)WINDOW_WIDTH,40*sc); g.DrawString(L"CONTROLS  [H/ESC to close]",-1,&tFnt,tRc,&sf,&gold);

    struct { const wchar_t* k; const wchar_t* d; } binds[]={
        {L"WASD / Arrows",L"Move"}, {L"Auto Fire",L"Continuous Shoot"},
        {L"Shift+Move",L"Speed x1.65"}, {L"Ctrl+Move",L"Slow x0.38"},
        {L"B",L"Bomb (clear screen)"}, {L"P",L"Pause/Resume"},
        {L"H",L"Help screen"}, {L"ESC",L"Back to Menu"},
        {L"Enter",L"Confirm"}, {L"A-Z",L"Enter name"},
    };
    const int nb=10; float c1=30*sc, c2=170*sc, rh=24*sc, sy=52*sc;
    for (int i=0;i<nb;i++) {
        float y=sy+i*rh;
        g.DrawString(binds[i].k,-1,&kFnt,RectF(c1,y,c2-6*sc,y+rh),&lf,&wh);
        g.DrawString(binds[i].d,-1,&dFnt,RectF(c2,y,(float)WINDOW_WIDTH-20*sc,y+rh),&lf,&dim);
    }

    float iy=sy+nb*rh+8*sc;
    g.DrawString(L"ITEMS",-1,&tFnt,RectF(0,iy,(float)WINDOW_WIDTH,iy+32*sc),&sf,&gold);
    struct { const wchar_t* i; const wchar_t* d; } itms[]={
        {L"[F] Orange",L"Firepower +1 (max 5)"},{L"[H] Red",L"+1 Life (max 5)"},
        {L"[B] Orange",L"+1 Bomb (max 3)"},{L"[S] Blue",L"Shield 10s"},
        {L"[R] Green",L"Fire Rate Up (8s)"},
    };
    const int ni=5;
    iy+=28*sc;
    for (int i=0;i<ni;i++) { float y=iy+i*rh; g.DrawString(itms[i].i,-1,&kFnt,RectF(c1,y,c2-6*sc,y+rh),&lf,&wh); g.DrawString(itms[i].d,-1,&dFnt,RectF(c2,y,(float)WINDOW_WIDTH-20*sc,y+rh),&lf,&dim); }

    RectF bRc(0,WINDOW_HEIGHT-16*sc,(float)WINDOW_WIDTH,WINDOW_HEIGHT-2*sc);
    g.DrawString(L"Press H or ESC to return to game",-1,&dFnt,bRc,&sf,&hnt);
}

// ========== 各实体渲染委托 ==========

void Game::renderEnemies(Graphics& g) { for (auto* e : enemies) if (e->alive) e->render(g); }
void Game::renderBullets(Graphics& g) { for (auto* b : playerBullets) if (b->alive) b->render(g); for (auto* b : enemyBullets) if (b->alive) b->render(g); }
void Game::renderItems(Graphics& g)   { for (auto* it : items) if (it->alive) it->render(g); }

// ========== 发光辅助 ==========

void Game::drawGlow(Graphics& g, float cx, float cy, float r, Color c, int layers) {
    for (int i=layers-1;i>=0;i--) {
        float a=1.0f-(float)i/layers*0.7f;
        Pen pn(Color((BYTE)(c.GetA()*a),c.GetR(),c.GetG(),c.GetB()),1+i*0.8f);
        g.DrawEllipse(&pn, cx-r-i*2, cy-r-i*2, (r+i*2)*2, (r+i*2)*2);
    }
}

// ── 辅助：窄字符串转宽字符串（栈缓冲区，避免 std::wstring ABI 问题） ──
static const wchar_t* widen(const char* s) {
    static wchar_t buf[256];
    if (!s) { buf[0] = L'\0'; return buf; }
    int len = MultiByteToWideChar(CP_UTF8, 0, s, -1, buf, 256);
    if (len <= 0) buf[0] = L'\0';
    return buf;
}
