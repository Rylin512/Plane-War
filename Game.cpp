#include "Game.h"
#include "Player.h"
#include "Enemy.h"
#include "Boss.h"
#include "Bullet.h"
#include "Item.h"
#include "Resource.h"
#ifndef __MINGW32__
#include "D2DRenderer.h"
#endif
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
double TARGET_FRAME_TIME = 1.0 / 60.0;

// Palette 颜色定义
namespace Palette {
    Gdiplus::Color DeepSpace(255,2,2,18), MenuTitle(255,255,220,0), MenuSel(255,255,255,140), MenuNormal(255,185,185,185), MenuHint(255,110,110,110);
    Gdiplus::Color PlayerBody(255,0,190,255), PlayerDark(255,0,120,200), PlayerWing(255,190,210,230), PlayerCockpit(255,0,248,255), PlayerEngine(255,255,165,20), PlayerGlow(255,0,140,240), PlayerFastGlow(255,0,255,140), PlayerSlowGlow(255,100,180,255);
    Gdiplus::Color EnemyNormal(255,225,38,38), EnemyNormalDark(255,160,18,18), EnemyFast(255,255,165,0), EnemyFastDark(255,200,120,0), EnemyShooting(255,165,38,225), EnemyShootingLit(255,225,105,255);
    Gdiplus::Color BossPhase1(255,185,22,22), BossPhase1Dark(255,110,10,10), BossPhase2(255,235,85,15), BossPhase2Dark(255,150,50,8), BossPhase3(255,255,20,20), BossPhase3Dark(255,170,10,10), BossCockpit(255,0,210,255);
    Gdiplus::Color BulletPlayer(255,255,255,60), BulletPlayerCore(255,255,255,210), BulletEnemy(255,255,45,45), BulletEnemyCore(255,255,160,160);
    Gdiplus::Color ItemFirepower(255,255,205,0), ItemHealth(255,255,55,55), ItemBomb(255,255,125,0), ItemShield(255,0,155,255);
    Gdiplus::Color ParticleExplosion(255,255,105,25), ParticleBomb(255,255,255,210);
}

// 5 种皮肤
SkinColors SKINS[] = {
    // 0: 经典蓝
    { L"Classic",
        Color(255,0,190,255), Color(255,0,120,200), Color(255,190,210,230), Color(255,0,248,255), Color(255,255,165,20), Color(255,0,140,240), Color(255,0,255,140), Color(255,100,180,255),
        Color(255,225,38,38), Color(255,160,18,18), Color(255,255,165,0), Color(255,200,120,0), Color(255,165,38,225), Color(255,225,105,255),
        Color(255,185,22,22), Color(255,110,10,10), Color(255,235,85,15), Color(255,150,50,8), Color(255,255,20,20), Color(255,170,10,10), Color(255,0,210,255),
        Color(255,255,255,60), Color(255,255,255,210), Color(255,255,45,45), Color(255,255,160,160),
        Color(255,255,205,0), Color(255,255,55,55), Color(255,255,125,0), Color(255,0,155,255),
        Color(255,255,105,25), Color(255,255,255,210) },
    // 1: 霓虹绿
    { L"Neon",
        Color(255,0,255,100), Color(255,0,180,60), Color(255,100,255,150), Color(255,0,255,200), Color(255,255,100,255), Color(255,0,255,100), Color(255,100,255,200), Color(255,0,200,100),
        Color(255,255,0,200), Color(255,180,0,140), Color(255,0,255,255), Color(255,0,180,180), Color(255,255,200,0), Color(255,255,255,100),
        Color(255,255,50,200), Color(255,180,20,140), Color(255,255,150,50), Color(255,180,100,20), Color(255,255,0,100), Color(255,180,0,60), Color(255,0,255,150),
        Color(255,200,255,100), Color(255,200,255,255), Color(255,255,50,200), Color(255,255,200,255),
        Color(255,0,255,100), Color(255,255,50,200), Color(255,255,150,0), Color(255,0,200,255),
        Color(255,255,0,200), Color(255,255,255,200) },
    // 2: 熔岩红
    { L"Lava",
        Color(255,255,80,20), Color(255,180,40,0), Color(255,255,140,60), Color(255,255,200,100), Color(255,255,200,0), Color(255,255,50,0), Color(255,255,120,0), Color(255,200,80,40),
        Color(255,255,200,0), Color(255,200,140,0), Color(255,255,80,0), Color(255,200,50,0), Color(255,255,100,50), Color(255,255,200,150),
        Color(255,255,60,0), Color(255,180,30,0), Color(255,255,140,0), Color(255,180,90,0), Color(255,255,0,0), Color(255,180,0,0), Color(255,255,180,50),
        Color(255,255,200,100), Color(255,255,255,200), Color(255,255,100,30), Color(255,255,200,150),
        Color(255,255,120,0), Color(255,255,50,0), Color(255,255,180,0), Color(255,255,100,50),
        Color(255,255,80,0), Color(255,255,255,150) },
    // 3: 极光紫
    { L"Aurora",
        Color(255,80,0,200), Color(255,40,0,140), Color(255,150,100,220), Color(255,200,100,255), Color(255,100,255,255), Color(255,80,0,200), Color(255,200,0,255), Color(255,100,50,200),
        Color(255,0,200,200), Color(255,0,140,140), Color(255,200,0,200), Color(255,140,0,140), Color(255,100,200,255), Color(255,200,255,255),
        Color(255,100,0,200), Color(255,60,0,140), Color(255,150,50,200), Color(255,100,20,140), Color(255,200,0,255), Color(255,140,0,180), Color(255,150,200,255),
        Color(255,200,150,255), Color(255,255,200,255), Color(255,0,200,200), Color(255,150,255,255),
        Color(255,200,0,200), Color(255,200,0,200), Color(255,150,0,200), Color(255,100,200,255),
        Color(255,100,0,255), Color(255,255,200,255) },
    // 4: 单色灰
    { L"Mono",
        Color(255,200,200,200), Color(255,120,120,120), Color(255,220,220,220), Color(255,255,255,255), Color(255,180,180,180), Color(255,150,150,150), Color(255,255,255,255), Color(255,100,100,100),
        Color(255,160,160,160), Color(255,100,100,100), Color(255,180,180,180), Color(255,120,120,120), Color(255,200,200,200), Color(255,255,255,255),
        Color(255,140,140,140), Color(255,80,80,80), Color(255,170,170,170), Color(255,100,100,100), Color(255,200,200,200), Color(255,120,120,120), Color(255,255,255,255),
        Color(255,255,255,255), Color(255,255,255,255), Color(255,150,150,150), Color(255,220,220,220),
        Color(255,200,200,200), Color(255,150,150,150), Color(255,180,180,180), Color(255,220,220,220),
        Color(255,180,180,180), Color(255,255,255,255) },
};
int SKIN_COUNT = 5;

void applySkin(int idx) {
    if (idx < 0 || idx >= SKIN_COUNT) return;
    SkinColors& s = SKINS[idx];
    Palette::PlayerBody = s.playerBody; Palette::PlayerDark = s.playerDark; Palette::PlayerWing = s.playerWing; Palette::PlayerCockpit = s.playerCockpit; Palette::PlayerEngine = s.playerEngine; Palette::PlayerGlow = s.playerGlow; Palette::PlayerFastGlow = s.playerFastGlow; Palette::PlayerSlowGlow = s.playerSlowGlow;
    Palette::EnemyNormal = s.enemyNormal; Palette::EnemyNormalDark = s.enemyNormalDark; Palette::EnemyFast = s.enemyFast; Palette::EnemyFastDark = s.enemyFastDark; Palette::EnemyShooting = s.enemyShooting; Palette::EnemyShootingLit = s.enemyShootingLit;
    Palette::BossPhase1 = s.bossP1; Palette::BossPhase1Dark = s.bossP1D; Palette::BossPhase2 = s.bossP2; Palette::BossPhase2Dark = s.bossP2D; Palette::BossPhase3 = s.bossP3; Palette::BossPhase3Dark = s.bossP3D; Palette::BossCockpit = s.bossCockpit;
    Palette::BulletPlayer = s.bulletPlayer; Palette::BulletPlayerCore = s.bulletPlayerCore; Palette::BulletEnemy = s.bulletEnemy; Palette::BulletEnemyCore = s.bulletEnemyCore;
    Palette::ItemFirepower = s.itemFire; Palette::ItemHealth = s.itemHealth; Palette::ItemBomb = s.itemBomb; Palette::ItemShield = s.itemShield;
    Palette::ParticleExplosion = s.particleExplosion; Palette::ParticleBomb = s.particleBomb;
}

static Game* g_game = nullptr;

// 前向声明
static const wchar_t* widen(const char* s);
Renderer* CreateGdiplusRenderer(Gdiplus::Graphics* graphics, int width, int height);
static bool clickIn(float mx, float my, float x, float y, float w, float h);
static void renderBackBtn(Renderer& r, float sc, const wchar_t* label = L"BACK");
static bool clickBackBtn(float mx, float my, float sc);

// ========== 窗口过程 ==========

LRESULT CALLBACK Game::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_DESTROY: PostQuitMessage(0); return 0;
    case WM_MOUSEMOVE:
        if (g_game) { g_game->mouseX = (float)LOWORD(lParam); g_game->mouseY = (float)HIWORD(lParam); }
        return 0;
    case WM_LBUTTONDOWN:
        if (g_game) { g_game->mouseX = (float)LOWORD(lParam); g_game->mouseY = (float)HIWORD(lParam); g_game->mouseClicked = true; }
        return 0;
    case WM_KEYDOWN:
        if (g_game && wParam == VK_ESCAPE) {
            if (g_game->state == GameState::HELP) { g_game->state = GameState::PLAYING; }
            else if (g_game->state == GameState::PLAYING) g_game->state = GameState::PAUSED;
            else if (g_game->state == GameState::PAUSED) {
                delete g_game->player; g_game->player = nullptr;
                for (auto* e : g_game->enemies) delete e;
                for (auto* b : g_game->playerBullets) { b->alive = false; g_game->releaseBullet(b); }
                for (auto* b : g_game->enemyBullets)  { b->alive = false; g_game->releaseBullet(b); }
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
    , mouseX(0), mouseY(0), mouseClicked(false)
    , state(GameState::MENU), menuSelection(MenuOption::START_GAME)
    , settingsSelection(0), currentResolution(0), currentRefreshRate(DEFAULT_REFRESH_RATE), stateTimer(0)
    , settingsPage(0), currentAspectRatio(AR_16_9), currentSkin(0), antiAlias(true), perfMode(false)
    , shakeTimer(0), shakeIntensity(0)
    , currentLevel(0), scoreToNextLevel(500), spawnInterval(40), fastEnemyChance(0.1f), shootingEnemyChance(0)
    , enemySpawnTimer(0), enemiesKilledThisLevel(0)
    , bossSpawned(false), bossDefeated(false)
    , score(0), highScore(0), player(nullptr), nameLength(0)
    , deltaTime(0), menuCooldown(0)
    , memDC(nullptr), memBmp(nullptr), oldBmp(nullptr), backGraphics(nullptr)
    , cachedBackground(nullptr)
    , ffConsolas(nullptr), ffArial(nullptr), fntHUD(nullptr), fntHUDSmall(nullptr)
    , brWhite(nullptr), brRed(nullptr), brOrange(nullptr), brYellow(nullptr), brCyan(nullptr)
    , brGray(nullptr), brDim(nullptr), brGold(nullptr), brSel(nullptr), brNor(nullptr)
    , brGreen(nullptr), brWarning(nullptr), brDarkOverlay(nullptr)
    , brParticleTrail(nullptr), brParticleMain(nullptr)
    , penSeparator(nullptr)
    , starBrushes{nullptr, nullptr, nullptr, nullptr}
    , renderer(nullptr), useDirect2D(true), d2dAvailable(false)
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
    loadSettings();  // 加载用户设置（在创建窗口前）
    WINDOW_WIDTH  = RESOLUTIONS[currentResolution].width;
    WINDOW_HEIGHT = RESOLUTIONS[currentResolution].height;
    GAME_SCALE    = WINDOW_HEIGHT / 480.0f;  // 全局缩放

    if (!createWindow(hInst)) return false;

    // 加载素材
    bgImage = Resource::LoadImage(Resource::BG_CIRCUIT);
    uiFrame = Resource::LoadImage(Resource::UI_FRAME);

    createRenderResources();
    initDirect2D();  // 探测 D2D 可用性
    applySkin(currentSkin);
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

    // 重建持久化渲染资源（分辨率变化）
    createRenderResources();
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
    // 限制星空数量，避免高分辨率下性能悬崖
    const int STAR_MAX = 400;
    if (cnt > STAR_MAX) cnt = STAR_MAX;
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
        if      (cr < 5)  { s.color = Color(255, 140, 160, 255); s.colorIndex = 0; }
        else if (cr < 10) { s.color = Color(255, 255, 220, 140); s.colorIndex = 1; }
        else if (cr < 13) { s.color = Color(255, 255, 160, 140); s.colorIndex = 2; }
        else              { s.color = Color(255, 200, 210, 255); s.colorIndex = 3; }
        stars.push_back(s);
    }
}

// ========== 持久化渲染资源 ==========

void Game::destroyRenderResources() {
    if (renderer) { renderer->shutdown(); delete renderer; renderer = nullptr; }
    if (backGraphics) { delete backGraphics; backGraphics = nullptr; }
    if (memDC) { SelectObject(memDC, oldBmp); DeleteObject(memBmp); DeleteDC(memDC); memDC = nullptr; }
    if (cachedBackground) { delete cachedBackground; cachedBackground = nullptr; }
    delete fntHUD; fntHUD = nullptr;
    delete fntHUDSmall; fntHUDSmall = nullptr;
    delete ffConsolas; ffConsolas = nullptr;
    delete ffArial; ffArial = nullptr;
    delete brWhite; delete brRed; delete brOrange; delete brYellow; delete brCyan;
    delete brGray; delete brDim; delete brGold; delete brSel; delete brNor;
    delete brGreen; delete brWarning; delete brDarkOverlay;
    delete brParticleTrail; delete brParticleMain;
    delete penSeparator;
    for (auto*& sb : starBrushes) { delete sb; sb = nullptr; }
}

void Game::recreateFonts() {
    delete fntHUD; fntHUD = nullptr;
    delete fntHUDSmall; fntHUDSmall = nullptr;
    delete ffConsolas; ffConsolas = nullptr;
    delete ffArial; ffArial = nullptr;

    float sc = (std::max)(0.8f, WINDOW_WIDTH / 640.0f);
    int fs = (int)(12 * sc), fsSm = (int)(9 * sc);
    ffConsolas = new FontFamily(L"Consolas");
    if (!ffConsolas->IsAvailable()) { delete ffConsolas; ffConsolas = new FontFamily(L"Arial"); }
    ffArial = new FontFamily(L"Arial");
    fntHUD = new Font(ffConsolas, (float)fs, FontStyleBold, UnitPixel);
    fntHUDSmall = new Font(ffConsolas, (float)fsSm, FontStyleRegular, UnitPixel);
}

void Game::createRenderResources() {
    destroyRenderResources();

    // 后台缓冲（GDI+ 后端需要）
    memDC = CreateCompatibleDC(frontDC);
    memBmp = CreateCompatibleBitmap(frontDC, WINDOW_WIDTH, WINDOW_HEIGHT);
    oldBmp = (HBITMAP)SelectObject(memDC, memBmp);
    backGraphics = new Graphics(memDC);

    // 默认渲染质量（用户可在设置中调整）
    backGraphics->SetSmoothingMode(SmoothingModeAntiAlias);
    backGraphics->SetTextRenderingHint(TextRenderingHintAntiAlias);

    // 字体（GDI+ 路径使用）
    recreateFonts();

    // 常用画刷
    brWhite  = new SolidBrush(Color::White);
    brRed    = new SolidBrush(Color(255, 255, 60, 60));
    brOrange = new SolidBrush(Color(255, 255, 165, 20));
    brYellow = new SolidBrush(Color(255, 255, 225, 40));
    brCyan   = new SolidBrush(Color(255, 0, 195, 255));
    brGray   = new SolidBrush(Color(255, 200, 200, 200));
    brDim    = new SolidBrush(Color(255, 140, 140, 140));
    brGold   = new SolidBrush(Color(255, 255, 220, 0));
    brSel    = new SolidBrush(Color(255, 255, 255, 140));
    brNor    = new SolidBrush(Color(255, 185, 185, 185));
    brGreen  = new SolidBrush(Color(255, 0, 255, 130));
    brWarning = new SolidBrush(Color(255, 255, 45, 45));
    brDarkOverlay = new SolidBrush(Color(160, 0, 0, 0));
    brParticleTrail = new SolidBrush(Color::Black);
    brParticleMain  = new SolidBrush(Color::Black);

    // 常用画笔
    penSeparator = new Pen(Color(255, 60, 60, 80), 1);

    // 星空画刷
    starBrushes[0] = new SolidBrush(Color(255, 140, 160, 255));
    starBrushes[1] = new SolidBrush(Color(255, 255, 220, 140));
    starBrushes[2] = new SolidBrush(Color(255, 255, 160, 140));
    starBrushes[3] = new SolidBrush(Color(255, 200, 210, 255));

    // 缓存背景平铺
    if (bgImage && bgImage->GetWidth() > 0 && bgImage->GetHeight() > 0) {
        cachedBackground = new Bitmap(WINDOW_WIDTH, WINDOW_HEIGHT);
        Graphics cacheG(cachedBackground);
        int iw = bgImage->GetWidth(), ih = bgImage->GetHeight();
        for (int y = 0; y < WINDOW_HEIGHT; y += ih)
            for (int x = 0; x < WINDOW_WIDTH; x += iw)
                cacheG.DrawImage(bgImage, x, y, iw, ih);
    }

    // 创建渲染器
    recreateRenderer();
}

void Game::initDirect2D() {
#ifdef __MINGW32__
    d2dAvailable = false;
#else
    if (d2dAvailable) return;
    Direct2DRenderer* d2d = new Direct2DRenderer();
    if (d2d->init(hwnd, WINDOW_WIDTH, WINDOW_HEIGHT)) {
        d2dAvailable = true;
    } else {
        delete d2d;
    }
#endif
}

void Game::recreateRenderer() {
    if (renderer) { renderer->shutdown(); delete renderer; renderer = nullptr; }

    if (useDirect2D) {
        initDirect2D();
        if (d2dAvailable) {
#ifndef __MINGW32__
            Direct2DRenderer* d2d = new Direct2DRenderer();
            if (d2d->init(hwnd, WINDOW_WIDTH, WINDOW_HEIGHT)) {
                renderer = d2d;
                return;
            }
            delete d2d;
#endif
        }
        // D2D 失败，回退 GDI+
        useDirect2D = false;
    }
    // 默认 GDI+ 后端
    renderer = CreateGdiplusRenderer(backGraphics, WINDOW_WIDTH, WINDOW_HEIGHT);
}

// ========== 子弹对象池 ==========

PlayerBullet* Game::acquirePlayerBullet(float x, float y) {
    if (!playerBulletPool.empty()) {
        PlayerBullet* b = playerBulletPool.back();
        playerBulletPool.pop_back();
        b->reset(x, y);
        return b;
    }
    return new PlayerBullet(x, y);
}

EnemyBullet* Game::acquireEnemyBullet(float x, float y, float angleX) {
    if (!enemyBulletPool.empty()) {
        EnemyBullet* b = enemyBulletPool.back();
        enemyBulletPool.pop_back();
        b->reset(x, y, angleX);
        return b;
    }
    return new EnemyBullet(x, y, angleX);
}

void Game::releaseBullet(Bullet* b) {
    if (!b) return;
    b->alive = false;
    if (b->getOwner() == BulletOwner::PLAYER) {
        playerBulletPool.push_back(static_cast<PlayerBullet*>(b));
    } else {
        enemyBulletPool.push_back(static_cast<EnemyBullet*>(b));
    }
}

// ========== 关闭 ==========

void Game::shutdown() {
    saveSettings();  // 持久化用户设置
    // 程序退出：快速清理，避免逐对象释放造成的卡顿
    delete player; player = nullptr;
    for (auto* e : enemies) delete e;
    for (auto* b : playerBullets) delete b;
    for (auto* b : enemyBullets) delete b;
    for (auto* i : items) delete i;
    enemies.clear(); playerBullets.clear(); enemyBullets.clear(); items.clear();
    particles.clear();
    delete bgImage; bgImage = nullptr;
    delete uiFrame; uiFrame = nullptr;
    // 子弹池：直接清空（OS 回收进程内存）
    playerBulletPool.clear(); enemyBulletPool.clear();
    // 渲染器快速关闭
    if (renderer) { renderer->shutdown(); delete renderer; renderer = nullptr; }
    if (backGraphics) { delete backGraphics; backGraphics = nullptr; }
    if (memDC) { SelectObject(memDC, oldBmp); DeleteObject(memBmp); DeleteDC(memDC); memDC = nullptr; }
    delete cachedBackground; cachedBackground = nullptr;
    // 跳过画刷/画笔逐个清理，进程退出时 OS 回收
    if (frontDC && hwnd) { ReleaseDC(hwnd, frontDC); frontDC = nullptr; }
    if (hwnd) { DestroyWindow(hwnd); hwnd = nullptr; }
}

// ========== 重置 ==========

void Game::resetGame() {
    delete player; player = nullptr;
    for (auto* e : enemies) delete e;
    for (auto* b : playerBullets) { b->alive = false; releaseBullet(b); }
    for (auto* b : enemyBullets)  { b->alive = false; releaseBullet(b); }
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
        if (!hwnd) break;  // 退出时立即跳出，跳过渲染和 busy-wait

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

// 鼠标点击辅助
static bool clickIn(float mx, float my, float x, float y, float w, float h) {
    return mx >= x && mx <= x + w && my >= y && my <= y + h;
}

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
        int mc = 5;
        if (up) { menuSelection = (MenuOption)(((int)menuSelection-1+mc)%mc); menuCooldown=8; }
        if (dn) { menuSelection = (MenuOption)(((int)menuSelection+1)%mc); menuCooldown=8; }
        if (enter) {
            menuCooldown=15;
            switch (menuSelection) {
            case MenuOption::START_GAME: resetGame(); break;
            case MenuOption::LEADERBOARD: state=GameState::LEADERBOARD; break;
            case MenuOption::SETTINGS_MENU:
                settingsPage = 0;
                currentAspectRatio = (int)RESOLUTIONS[currentResolution].aspect;
                settingsSelection = 0;
                for (int i = 0; i < currentResolution; i++)
                    if (RESOLUTIONS[i].aspect == RESOLUTIONS[currentResolution].aspect)
                        settingsSelection++;
                state = GameState::SETTINGS;
                break;
            case MenuOption::SKIN: state = GameState::SKIN_SELECT; break;
            case MenuOption::EXIT: PostQuitMessage(0); hwnd=nullptr; break;
            }
        }
    }

    // ── 设置（多页） ──
    if (state == GameState::SETTINGS && menuCooldown <= 0) {
        bool up   = (GetAsyncKeyState(VK_UP)&0x8000)||(GetAsyncKeyState('W')&0x8000);
        bool dn   = (GetAsyncKeyState(VK_DOWN)&0x8000)||(GetAsyncKeyState('S')&0x8000);
        bool left = (GetAsyncKeyState(VK_LEFT)&0x8000)||(GetAsyncKeyState('A')&0x8000);
        bool right= (GetAsyncKeyState(VK_RIGHT)&0x8000)||(GetAsyncKeyState('D')&0x8000);
        bool enter= (GetAsyncKeyState(VK_RETURN)&0x8000)!=0;
        bool esc  = (GetAsyncKeyState(VK_ESCAPE)&0x8000)!=0;

        // 翻页（仅 Tab 键）
        bool tab = (GetAsyncKeyState(VK_TAB)&0x8000)!=0;
        if (tab) { settingsPage=(settingsPage+1)%3; settingsSelection=0; menuCooldown=10; }

        // 当前页操作
        if (settingsPage == 0) {  // 分辨率（宽高比子导航）
            // Left/Right 切换宽高比类别
            if (left)  { currentAspectRatio=(currentAspectRatio-1+AR_COUNT)%AR_COUNT; settingsSelection=0; menuCooldown=10; }
            if (right) { currentAspectRatio=(currentAspectRatio+1)%AR_COUNT;       settingsSelection=0; menuCooldown=10; }
            // 统计当前宽高比下有多少选项
            int cnt = 0;
            for (int i = 0; i < RESOLUTION_COUNT; i++)
                if (RESOLUTIONS[i].aspect == (AspectRatio)currentAspectRatio) cnt++;
            if (cnt > 0) {
                if (up)   { settingsSelection=(settingsSelection-1+cnt)%cnt; menuCooldown=8; }
                if (dn)   { settingsSelection=(settingsSelection+1)%cnt; menuCooldown=8; }
                if (enter) {
                    int applyIdx = 0;
                    for (int i = 0, n = 0; i < RESOLUTION_COUNT; i++) {
                        if (RESOLUTIONS[i].aspect == (AspectRatio)currentAspectRatio) {
                            if (n == settingsSelection) { applyIdx = i; break; }
                            n++;
                        }
                    }
                    applyResolution(applyIdx); state=GameState::MENU; menuCooldown=15;
                }
            }
        } else if (settingsPage == 1) {  // 刷新率
            if (up)   { settingsSelection=(settingsSelection-1+REFRESH_RATE_COUNT)%REFRESH_RATE_COUNT; menuCooldown=8; }
            if (dn)   { settingsSelection=(settingsSelection+1)%REFRESH_RATE_COUNT; menuCooldown=8; }
            if (enter) {
                currentRefreshRate = settingsSelection;
                TARGET_FRAME_TIME = 1.0 / REFRESH_RATES[currentRefreshRate];
                menuCooldown = 15;
            }
        } else {  // 渲染选项
            int optCount = d2dAvailable ? 3 : 2;
            if (up)   { settingsSelection=(settingsSelection-1+optCount)%optCount; menuCooldown=8; }
            if (dn)   { settingsSelection=(settingsSelection+1)%optCount; menuCooldown=8; }
            if (enter) {
                if (settingsSelection == 0) antiAlias = !antiAlias;
                else if (settingsSelection == 1) perfMode = !perfMode;
                else if (settingsSelection == 2) { useDirect2D = !useDirect2D; recreateRenderer(); }
                menuCooldown = 12;
            }
        }
        if (esc) { settingsPage=0; settingsSelection=0; state=GameState::MENU; menuCooldown=15; }
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

    // ── 皮肤选择 ──
    if (state == GameState::SKIN_SELECT && menuCooldown <= 0) {
        bool left = (GetAsyncKeyState(VK_LEFT)&0x8000)||(GetAsyncKeyState('A')&0x8000);
        bool right= (GetAsyncKeyState(VK_RIGHT)&0x8000)||(GetAsyncKeyState('D')&0x8000);
        bool enter= (GetAsyncKeyState(VK_RETURN)&0x8000)!=0;
        bool esc  = (GetAsyncKeyState(VK_ESCAPE)&0x8000)!=0;
        if (left)  { currentSkin = (currentSkin - 1 + SKIN_COUNT) % SKIN_COUNT; applySkin(currentSkin); menuCooldown = 10; }
        if (right) { currentSkin = (currentSkin + 1) % SKIN_COUNT; applySkin(currentSkin); menuCooldown = 10; }
        if (enter || esc) { state = GameState::MENU; menuCooldown = 15; }
    }

    // ── 鼠标点击统一处理 ──
    if (!mouseClicked) return;
    mouseClicked = false;
    float mx = mouseX, my = mouseY;

    if (state == GameState::MENU) {
        float sc = (std::min)(WINDOW_HEIGHT / 480.0f, 2.2f);
        int itemCount = 5;
        float optStart = 160 * sc, optStep = 34 * sc;
        for (int i = 0; i < itemCount; i++) {
            if (clickIn(mx, my, 0, optStart + i * optStep, (float)WINDOW_WIDTH, optStep)) {
                menuSelection = (MenuOption)i;
                switch (i) {
                case 0: resetGame(); break;
                case 1: state = GameState::LEADERBOARD; break;
                case 2: settingsPage = 0; currentAspectRatio = (int)RESOLUTIONS[currentResolution].aspect; settingsSelection = 0; for (int j = 0; j < currentResolution; j++) if (RESOLUTIONS[j].aspect == RESOLUTIONS[currentResolution].aspect) settingsSelection++; state = GameState::SETTINGS; break;
                case 3: state = GameState::SKIN_SELECT; break;
                case 4: PostQuitMessage(0); hwnd = nullptr; break;
                }
                return;
            }
        }
    }

    if (state == GameState::SETTINGS) {
        float sc = (std::min)(WINDOW_HEIGHT / 480.0f, 2.2f);
        if (clickBackBtn(mx, my, sc)) { state = GameState::MENU; return; }
        // 点击页标签（左侧竖排）
        float tabY = 68 * sc, tabH = 22 * sc;
        for (int i = 0; i < 3; i++) {
            if (clickIn(mx, my, 0, tabY + i * tabH, (float)WINDOW_WIDTH, tabH)) {
                settingsPage = i; settingsSelection = 0; menuCooldown = 10; return;
            }
        }
        float sepY = tabY + 3 * tabH + 2 * sc;
        float contentY = sepY + 8 * sc;
        if (settingsPage == 0) {
            // 宽高比标签
            float arTabW = WINDOW_WIDTH / (float)AR_COUNT;
            for (int a = 0; a < AR_COUNT; a++) {
                if (clickIn(mx, my, a * arTabW, contentY, arTabW, 20 * sc)) {
                    currentAspectRatio = a; settingsSelection = 0; menuCooldown = 10; return;
                }
            }
            // 分辨率列表
            float optStart = contentY + 28 * sc, optStep = 28 * sc;
            int idx = 0;
            for (int i = 0; i < RESOLUTION_COUNT; i++) {
                if (RESOLUTIONS[i].aspect != (AspectRatio)currentAspectRatio) continue;
                if (clickIn(mx, my, 50 * sc, optStart + idx * optStep, WINDOW_WIDTH - 50 * sc, optStep)) {
                    applyResolution(i); state = GameState::MENU; menuCooldown = 15; return;
                }
                idx++;
            }
        } else if (settingsPage == 1) {
            float optStep = 32 * sc;
            for (int i = 0; i < REFRESH_RATE_COUNT; i++) {
                if (clickIn(mx, my, 50 * sc, contentY + i * optStep, WINDOW_WIDTH - 50 * sc, optStep)) {
                    currentRefreshRate = i; TARGET_FRAME_TIME = 1.0 / REFRESH_RATES[i]; menuCooldown = 15; return;
                }
            }
        } else {
            float optStep = 32 * sc;
            if (clickIn(mx, my, 50 * sc, contentY, WINDOW_WIDTH - 50 * sc, optStep)) { antiAlias = !antiAlias; menuCooldown = 12; return; }
            if (clickIn(mx, my, 50 * sc, contentY + optStep, WINDOW_WIDTH - 50 * sc, optStep)) { perfMode = !perfMode; menuCooldown = 12; return; }
            if (d2dAvailable && clickIn(mx, my, 50 * sc, contentY + 2 * optStep, WINDOW_WIDTH - 50 * sc, optStep)) { useDirect2D = !useDirect2D; recreateRenderer(); menuCooldown = 12; return; }
        }
    }

    if (state == GameState::PAUSED) {
        float sc = (std::min)(WINDOW_HEIGHT / 480.0f, 2.2f);
        if (clickBackBtn(mx, my, sc)) { state = GameState::PLAYING; return; }
        state = GameState::PLAYING; return;
    }

    if (state == GameState::HELP) {
        float sc = (std::min)(WINDOW_HEIGHT / 480.0f, 2.2f);
        if (clickBackBtn(mx, my, sc)) { state = GameState::PLAYING; return; }
        state = GameState::PLAYING; return;
    }

    if (state == GameState::GAME_OVER || state == GameState::VICTORY) {
        float sc = (std::min)(WINDOW_HEIGHT / 480.0f, 2.2f);
        if (clickBackBtn(mx, my, sc)) { state = GameState::MENU; return; }
        if (isHighScore()) { nameLength = 0; memset(nameBuffer, 0, sizeof(nameBuffer)); state = GameState::NAME_ENTRY; }
        else state = GameState::MENU;
        return;
    }

    if (state == GameState::LEADERBOARD) {
        float sc = (std::min)(WINDOW_HEIGHT / 480.0f, 2.2f);
        if (clickBackBtn(mx, my, sc)) { state = GameState::MENU; return; }
        state = GameState::MENU; return;
    }

    if (state == GameState::NAME_ENTRY) {
        float sc = (std::min)(WINDOW_HEIGHT / 480.0f, 2.2f);
        if (clickBackBtn(mx, my, sc)) { state = GameState::MENU; return; }
    }

    if (state == GameState::LEVEL_TRANSITION) {
        stateTimer = 0; state = GameState::PLAYING; return;
    }

    if (state == GameState::SKIN_SELECT) {
        float sc = (std::min)(WINDOW_HEIGHT / 480.0f, 2.2f);
        if (clickBackBtn(mx, my, sc)) { state = GameState::MENU; return; }
        float btnW = 120 * sc, btnH = 35 * sc;
        float cy = WINDOW_HEIGHT / 2.0f + 80 * sc;
        if (clickIn(mx, my, WINDOW_WIDTH/2.0f - btnW - 20*sc, cy, btnW, btnH)) {
            currentSkin = (currentSkin - 1 + SKIN_COUNT) % SKIN_COUNT; applySkin(currentSkin); return;
        }
        if (clickIn(mx, my, WINDOW_WIDTH/2.0f + 20*sc, cy, btnW, btnH)) {
            currentSkin = (currentSkin + 1) % SKIN_COUNT; applySkin(currentSkin); return;
        }
        state = GameState::MENU; return;
    }
}

// ========== 更新 ==========

void Game::update() {
    if (state != GameState::PLAYING && state != GameState::LEVEL_TRANSITION) return;

    if (shakeTimer > 0 && !perfMode) shakeTimer--;

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
            if (lv>=1) playerBullets.push_back(acquirePlayerBullet(cx-BULLET_PLAYER_WIDTH*sc/2.0f, by));
            if (lv>=2) { playerBullets.push_back(acquirePlayerBullet(cx-BULLET_PLAYER_WIDTH*sc/2.0f-8*sc,by+4*sc)); playerBullets.push_back(acquirePlayerBullet(cx-BULLET_PLAYER_WIDTH*sc/2.0f+8*sc,by+4*sc)); }
            if (lv>=3) { playerBullets.push_back(acquirePlayerBullet(cx-BULLET_PLAYER_WIDTH*sc/2.0f-14*sc,by+8*sc)); playerBullets.push_back(acquirePlayerBullet(cx-BULLET_PLAYER_WIDTH*sc/2.0f+14*sc,by+8*sc)); }
            if (lv>=4) { playerBullets.push_back(acquirePlayerBullet(cx-BULLET_PLAYER_WIDTH*sc/2.0f-4*sc,by-4*sc)); playerBullets.push_back(acquirePlayerBullet(cx-BULLET_PLAYER_WIDTH*sc/2.0f+4*sc,by-4*sc)); }
            if (lv>=5) { playerBullets.push_back(acquirePlayerBullet(cx-BULLET_PLAYER_WIDTH*sc/2.0f-10*sc,by-4*sc)); playerBullets.push_back(acquirePlayerBullet(cx-BULLET_PLAYER_WIDTH*sc/2.0f+10*sc,by-4*sc)); }
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
            enemyBullets.push_back(acquireEnemyBullet(e->centerX()-3, e->y+e->height, ax));
            // Lv3+ 射击敌机 3 连发
            if (currentLevel>=3 && e->shootInterval<=30) {
                enemyBullets.push_back(acquireEnemyBullet(e->centerX()-3-6, e->y+e->height+4, ax-0.1f));
                enemyBullets.push_back(acquireEnemyBullet(e->centerX()-3+6, e->y+e->height+4, ax+0.1f));
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
        if ((*it)->y<-20||(*it)->y>WINDOW_HEIGHT+20) { releaseBullet(*it); it=playerBullets.erase(it); } else ++it;
    }
    for (auto it=enemyBullets.begin(); it!=enemyBullets.end();) {
        if ((*it)->y<-20||(*it)->y>WINDOW_HEIGHT+20) { releaseBullet(*it); it=enemyBullets.erase(it); } else ++it;
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
    for (auto* b:enemyBullets) releaseBullet(b);
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
    int limit = perfMode ? (PARTICLE_MAX / 4) : PARTICLE_MAX;  // 性能模式粒子 1/4
    for (int i=0; i<cnt && (int)particles.size()<limit; i++) {
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
    for (auto* b:enemyBullets) releaseBullet(b);
    enemyBullets.clear();
    enemies.push_back(new Boss(WINDOW_WIDTH/2.0f - BOSS_WIDTH*GAME_SCALE/2.0f, currentLevel));
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

// ========== 设置持久化 ==========

void Game::saveSettings() {
    char path[MAX_PATH]; GetModuleFileNameA(nullptr, path, MAX_PATH);
    char* sl = strrchr(path, '\\'); if (sl) *(sl + 1) = '\0';
    strcat_s(path, sizeof(path), "settings.cfg");
    HANDLE h = CreateFileA(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return;

    DWORD bw;
    int vals[] = { currentResolution, currentRefreshRate, antiAlias ? 1 : 0, perfMode ? 1 : 0, useDirect2D ? 1 : 0, currentSkin };
    WriteFile(h, vals, sizeof(vals), &bw, nullptr);
    CloseHandle(h);
}

void Game::loadSettings() {
    char path[MAX_PATH]; GetModuleFileNameA(nullptr, path, MAX_PATH);
    char* sl = strrchr(path, '\\'); if (sl) *(sl + 1) = '\0';
    strcat_s(path, sizeof(path), "settings.cfg");
    HANDLE h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return;

    DWORD br;
    int vals[6] = {};
    if (ReadFile(h, vals, sizeof(vals), &br, nullptr) && br >= 5 * (DWORD)sizeof(int)) {
        currentResolution = (std::max)(0, (std::min)(vals[0], RESOLUTION_COUNT - 1));
        currentRefreshRate = (std::max)(0, (std::min)(vals[1], REFRESH_RATE_COUNT - 1));
        antiAlias = vals[2] != 0;
        perfMode = vals[3] != 0;
        useDirect2D = vals[4] != 0;
        if (br >= 6 * (DWORD)sizeof(int)) currentSkin = (std::max)(0, (std::min)(vals[5], SKIN_COUNT - 1));
        TARGET_FRAME_TIME = 1.0 / REFRESH_RATES[currentRefreshRate];
    }
    CloseHandle(h);
}

// ========================================================================
//  渲染 — 全部使用 GDI+
// ========================================================================

void Game::render() {
    if (!renderer) return;

    renderer->beginFrame();
    renderer->clear(Color(255, 2, 2, 18));

    // 根据渲染选项动态调整质量
    if (perfMode) {
        renderer->setPerfMode(true);
    } else {
        renderer->setSmoothing(antiAlias);
    }

    switch (state) {
    case GameState::MENU:       renderMenu(*renderer); break;
    case GameState::SETTINGS:   renderSettings(*renderer); break;
    case GameState::PLAYING:
    case GameState::PAUSED:
    case GameState::LEVEL_TRANSITION:
        renderBackground(*renderer);
        renderParticles(*renderer);
        renderItems(*renderer);
        renderEnemies(*renderer);
        for (auto* e : enemies)
            if (e->getType()==EnemyType::BOSS && e->alive)
                static_cast<Boss*>(e)->renderHPBar(*renderer);
        renderBullets(*renderer);
        if (player) player->render(*renderer);
        renderHUD(*renderer);
        if (state==GameState::PAUSED) renderPauseOverlay(*renderer);
        if (state==GameState::LEVEL_TRANSITION) renderLevelTransition(*renderer);
        break;
    case GameState::HELP:       renderBackground(*renderer); renderParticles(*renderer); renderItems(*renderer); renderEnemies(*renderer); renderBullets(*renderer); if(player) player->render(*renderer); renderHUD(*renderer); renderHelp(*renderer); break;
    case GameState::GAME_OVER:
    case GameState::VICTORY:    renderGameOver(*renderer); break;
    case GameState::LEADERBOARD: renderLeaderboard(*renderer); break;
    case GameState::NAME_ENTRY:  renderNameEntry(*renderer); break;
    case GameState::SKIN_SELECT: renderSkinSelect(*renderer); break;
    }

    renderer->endFrame();

    // GDI+ 后端额外需要 BitBlt 到前台 DC
    if (!useDirect2D) {
        BitBlt(frontDC, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, memDC, 0, 0, SRCCOPY);
    }
}

// ========== 平铺背景（带亮度控制） ==========

void Game::renderTiledBG(Renderer& r, Image* img) {
    if (!img) return;
    int iw = img->GetWidth(), ih = img->GetHeight();
    if (iw<=0||ih<=0) return;
    for (int y=0; y<WINDOW_HEIGHT; y+=ih)
        for (int x=0; x<WINDOW_WIDTH; x+=iw)
            r.drawBitmap(img, (float)x, (float)y, (float)iw, (float)ih);
    // 亮度控制：暗色叠加层
    if (bgAlpha < 255) {
        int a = 255 - bgAlpha;
        if (a > 0) {
            r.fillRect(0.0f, 0.0f, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT, Color((BYTE)a, 0, 0, 0));
        }
    }
}

void Game::renderBackground(Renderer& r) {
    // 使用缓存的平铺背景
    if (cachedBackground) {
        r.drawBitmap(cachedBackground, 0, 0, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT);
    } else {
        renderTiledBG(r, bgImage);
    }
    // 亮度控制：暗色叠加层
    if (bgAlpha < 255) {
        int a = 255 - bgAlpha;
        if (a > 0) {
            r.fillRect(0.0f, 0.0f, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT, Color((BYTE)a, 0, 0, 0));
        }
    }
    // 星空叠加
    for (auto& s : stars) {
        s.twinkleTimer++;
        int br = s.brightness;
        if (!perfMode && s.size >= 2) { float tw = sinf((s.twinkleTimer + s.twinklePhase) * 0.05f); br = (int)(br * (0.6f + 0.4f * tw)); }
        br = (std::max)(20, (std::min)(255, br));
        Color starColor = s.color;
        Color c(starColor.GetA(), (BYTE)(starColor.GetR() * br / 255), (BYTE)(starColor.GetG() * br / 255), (BYTE)(starColor.GetB() * br / 255));
        if (s.size >= 2) {
            r.fillRect((int)s.x, (int)s.y, 2.0f, 2.0f, c);
            if (s.size >= 3 && br > 180) {
                r.fillRect((int)s.x + 2, (int)s.y, 1.0f, 1.0f, c);
                r.fillRect((int)s.x - 2, (int)s.y, 1.0f, 1.0f, c);
                r.fillRect((int)s.x, (int)s.y + 2, 1.0f, 1.0f, c);
                r.fillRect((int)s.x, (int)s.y - 2, 1.0f, 1.0f, c);
            }
        } else { r.fillRect((int)s.x, (int)s.y, 1.0f, 1.0f, c); }
    }
}

// ========== 粒子 ==========

void Game::renderParticles(Renderer& r) {
    for (auto& p : particles) {
        if (!p.alive) continue;
        float alpha = (float)p.lifetime / p.maxLifetime;
        // 拖尾
        Color tc(p.trailColor.GetA(), (BYTE)(p.trailColor.GetR()*alpha*0.4f), (BYTE)(p.trailColor.GetG()*alpha*0.4f), (BYTE)(p.trailColor.GetB()*alpha*0.4f));
        r.fillRect(p.x-p.vx, p.y-p.vy, (std::max)(1.0f,p.size*0.7f), (std::max)(1.0f,p.size*0.7f), tc);
        // 主体
        Color mc(p.color.GetA(), (BYTE)(p.color.GetR()*alpha), (BYTE)(p.color.GetG()*alpha), (BYTE)(p.color.GetB()*alpha));
        r.fillRect(p.x, p.y, (std::max)(1.0f,p.size), (std::max)(1.0f,p.size), mc);
    }
}

// ========== HUD ==========

void Game::renderHUD(Renderer& r) {
    if (!player) return;

    // 字体缩放：限制最大值，避免高分辨率下文字过大
    float scW = (std::max)(0.7f, (std::min)(WINDOW_WIDTH / 640.0f, 2.2f));
    float scH = (std::max)(0.7f, (std::min)(WINDOW_HEIGHT / 480.0f, 2.2f));
    float sc = (scW + scH) / 2.0f;
    int fs = (int)(12 * sc), fsSm = (int)(9 * sc);
    float lineH = 18 * sc;  // 行高
    const wchar_t* ff = L"Consolas";

    char buf[128];
    float pad = 8 * sc;

    // ── 左上信息 ──
    float yPos = pad;

    // 生命值条（独立一行）
    float lifeBarW = 100 * sc, lifeBarH = 8 * sc;
    r.fillRect(pad, yPos + (lineH - lifeBarH) / 2, lifeBarW, lifeBarH, Color(255, 40, 40, 40));
    float lifePct = (float)player->lives / PLAYER_MAX_LIVES;
    Color lifeC = lifePct > 0.5f ? Color(255, 60, 220, 60) : (lifePct > 0.25f ? Color(255, 255, 200, 40) : Color(255, 255, 60, 60));
    r.fillRect(pad, yPos + (lineH - lifeBarH) / 2, lifeBarW * lifePct, lifeBarH, lifeC);
    r.drawRect(pad, yPos + (lineH - lifeBarH) / 2, lifeBarW, lifeBarH, Color(255, 150, 150, 150), 1);
    sprintf_s(buf, sizeof(buf), "HP %d/%d", player->lives, PLAYER_MAX_LIVES);
    r.drawText(widen(buf), pad + lifeBarW + 6 * sc, yPos, ff, (float)fs, FontStyleBold, lifeC);
    yPos += lineH;

    sprintf_s(buf, sizeof(buf), "SCORE %d", score);
    r.drawText(widen(buf), pad, yPos, ff, (float)fs, FontStyleBold, Color::White);
    yPos += lineH;
    sprintf_s(buf, sizeof(buf), "BOMB %d", player->bombCount);
    r.drawText(widen(buf), pad, yPos, ff, (float)fs, FontStyleBold, Color(255, 255, 165, 20));
    yPos += lineH;
    sprintf_s(buf, sizeof(buf), "PWR Lv.%d", player->firepowerLevel);
    r.drawText(widen(buf), pad, yPos, ff, (float)fs, FontStyleBold, Color(255, 255, 225, 40));
    yPos += lineH;
    if (player->shieldActive) {
        sprintf_s(buf, sizeof(buf), "SHD %ds", player->shieldTimer / 60);
        r.drawText(widen(buf), pad, yPos, ff, (float)fs, FontStyleBold, Color(255, 0, 195, 255));
        yPos += lineH;
    }
    if (player->fireRateBoost) {
        sprintf_s(buf, sizeof(buf), "RAPID %ds", player->fireRateTimer / 60);
        r.drawText(widen(buf), pad, yPos, ff, (float)fs, FontStyleBold, Color(255, 0, 255, 120));
    }

    // ── 右上信息（右对齐） ──
    float rxW = 160 * sc;
    float rx = WINDOW_WIDTH - rxW - pad;
    yPos = pad;
    sprintf_s(buf, sizeof(buf), "LV %d", currentLevel);
    r.drawTextLeft(widen(buf), rx, yPos, rxW, lineH, ff, (float)fs, FontStyleBold, Color(255, 200, 200, 200));
    yPos += lineH;
    if (!bossSpawned) {
        sprintf_s(buf, sizeof(buf), "NXT %d", scoreToNextLevel);
        r.drawTextLeft(widen(buf), rx, yPos, rxW, lineH, ff, (float)fs, FontStyleBold, Color(255, 200, 200, 200));
    } else {
        r.drawTextLeft(widen("[BOSS]"), rx, yPos, rxW, lineH, ff, (float)fs, FontStyleBold, Color(255, 255, 45, 45));
    }
    yPos += lineH;
    sprintf_s(buf, sizeof(buf), "HST %d", (int)enemies.size());
    r.drawTextLeft(widen(buf), rx, yPos, rxW, lineH, ff, (float)fs, FontStyleBold, Color(255, 140, 140, 140));
    yPos += lineH;
    sprintf_s(buf, sizeof(buf), "BST %d", highScore);
    r.drawTextLeft(widen(buf), rx, yPos, rxW, lineH, ff, (float)fs, FontStyleBold, Color(255, 140, 140, 140));

    // ── 速度指示（屏幕中下方） ──
    if (keyShift || keyCtrl) {
        float midX = WINDOW_WIDTH / 2.0f;
        r.drawTextCentered(widen(keyShift ? "SPEED" : "SLOW"), midX - 50, WINDOW_HEIGHT - lineH * 2.5f,
                           100.0f, lineH, ff, (float)fs, FontStyleBold,
                           keyShift ? Color(255, 0, 255, 100) : Color(255, 100, 200, 255));
    }

    // ── 底部按键提示条（居中） ──
    float barH = lineH * 1.2f;
    float barY = WINDOW_HEIGHT - barH - pad;
    r.drawTextCentered(widen("[H] Help  [Shift] Speed  [Ctrl] Slow  [B] Bomb  [P] Pause  AutoFire ON"),
                       0, barY, (float)WINDOW_WIDTH, barH, ff, (float)fsSm, FontStyleRegular, Color(180, 160, 160, 160));
}

// ========== 菜单 ==========

void Game::renderMenu(Renderer& r) {
    float sc = (std::min)(WINDOW_HEIGHT / 480.0f, 2.2f);
    int tyOff = (int)(sinf(frameCount * 0.03f) * 3);
    const wchar_t* ff = L"Arial";

    r.drawTextCentered(L"PLANE WAR", 0, 35*sc+tyOff, (float)WINDOW_WIDTH, 80*sc,
                       ff, 42*sc, FontStyleBold, Color(255,255,220,0));
    r.drawTextCentered(L"GDI+ Edition | 60 FPS | H=Help", 0, 120*sc, (float)WINDOW_WIDTH, 20*sc,
                       ff, 12*sc, FontStyleRegular, Color(255,150,150,150));

    float lineY = 155 * sc;
    r.drawLine(WINDOW_WIDTH/2.0f-100*sc, lineY, WINDOW_WIDTH/2.0f+100*sc, lineY, Color(255,60,60,80), 1);

    const wchar_t* items[] = { L"START GAME", L"LEADERBOARD", L"SETTINGS", L"SKIN", L"EXIT" };
    int itemCount = 5;
    float optStart = 160 * sc, optStep = 34 * sc;
    for (int i = 0; i < itemCount; i++) {
        Color col = (int)menuSelection == i ? Color(255,255,255,140) : Color(255,185,185,185);
        wchar_t buf[80];
        if (i == 3)  // SKIN 显示当前皮肤名
            swprintf_s(buf, _countof(buf), (int)menuSelection == i ? L">  %s: %s" : L"   %s: %s", items[i], SKINS[currentSkin].name);
        else
            swprintf_s(buf, _countof(buf), (int)menuSelection == i ? L">  %s" : L"   %s", items[i]);
        r.drawTextCentered(buf, 0, optStart+i*optStep, (float)WINDOW_WIDTH, optStep,
                           ff, 22*sc, FontStyleRegular, col);
    }

    float hintY = optStart + itemCount * optStep + 8 * sc;
    r.drawTextCentered(L"WASD Move  AutoFire  Shift Speed  Ctrl Slow  B Bomb  P Pause  H Help",
                       0, hintY, (float)WINDOW_WIDTH, 20*sc, ff, 10*sc, FontStyleRegular, Color(255,110,110,110));
}

// ========== 设置 ==========

void Game::renderSettings(Renderer& r) {
    float sc = (std::min)(WINDOW_HEIGHT / 480.0f, 2.2f);
    const wchar_t* ff = L"Arial";
    renderBackBtn(r, sc);

    // 标题 + 页码
    wchar_t title[64];
    swprintf_s(title, _countof(title), L"SETTINGS  [Page %d/3]", settingsPage + 1);
    r.drawTextCentered(title, 0, 25*sc, (float)WINDOW_WIDTH, 37*sc, ff, 26*sc, FontStyleBold, Color(255,255,220,0));

    // 页标签栏
    const wchar_t* pages[] = { L"Resolution", L"Refresh Rate", L"Render Options" };
    float tabY = 68 * sc, tabH = 22 * sc;
    for (int i = 0; i < 3; i++) {
        Color col = (i == settingsPage) ? Color(255,255,220,0) : Color(255,140,140,140);
        wchar_t tab[32];
        swprintf_s(tab, _countof(tab), i == settingsPage ? L"< %s >" : L"  %s", pages[i]);
        r.drawTextCentered(tab, 0, tabY+i*tabH, (float)WINDOW_WIDTH, tabH, ff, 14*sc, FontStyleBold, col);
    }

    // 分隔线
    float sepY = tabY + 3 * tabH + 2 * sc;
    r.drawLine(40*sc, sepY, WINDOW_WIDTH-40*sc, sepY, Color(255,60,60,80), 1);

    float contentY = sepY + 8 * sc;

    if (settingsPage == 0) {
        // 宽高比标签行
        float arTabW = WINDOW_WIDTH / (float)AR_COUNT;
        for (int a = 0; a < AR_COUNT; a++) {
            Color arCol = (a == currentAspectRatio) ? Color(255, 255, 220, 0) : Color(255, 140, 140, 140);
            wchar_t arBuf[16];
            swprintf_s(arBuf, _countof(arBuf), a == currentAspectRatio ? L"< %hs >" : L" %hs", ASPECT_LABELS[a]);
            r.drawTextCentered(arBuf, a * arTabW, contentY, arTabW, 20 * sc, ff, 12 * sc, FontStyleBold, arCol);
        }

        // 分隔线
        float arSepY = contentY + 22 * sc;
        r.drawLine(40 * sc, arSepY, WINDOW_WIDTH - 40 * sc, arSepY, Color(255, 60, 60, 80), 1);

        // 当前宽高比下的分辨率列表
        float optStart = arSepY + 6 * sc, optStep = 28 * sc;
        int idx = 0;
        for (int i = 0; i < RESOLUTION_COUNT; i++) {
            if (RESOLUTIONS[i].aspect != (AspectRatio)currentAspectRatio) continue;
            bool sel = (idx == settingsSelection), act = (i == currentResolution);
            Color col = (sel && act) ? Color(255, 0, 255, 130) : (sel ? Color(255, 255, 255, 140) : (act ? Color(255, 0, 255, 130) : Color(255, 185, 185, 185)));
            wchar_t buf[80];
            swprintf_s(buf, _countof(buf), sel ? L"> %hs  %hs" : L"   %hs  %hs",
                       RESOLUTIONS[i].label, act ? L"[ACTIVE]" : L"");
            r.drawTextLeft(buf, 50 * sc, optStart + idx * optStep, WINDOW_WIDTH - 50 * sc, optStep, ff, 14 * sc, FontStyleRegular, col);
            idx++;
        }
    } else if (settingsPage == 1) {
        float optStep = 32 * sc;
        for (int i = 0; i < REFRESH_RATE_COUNT; i++) {
            bool sel = (i == settingsSelection), act = (i == currentRefreshRate);
            Color col = (sel && act) ? Color(255,0,255,130) : (sel ? Color(255,255,255,140) : (act ? Color(255,0,255,130) : Color(255,185,185,185)));
            wchar_t buf[80];
            swprintf_s(buf, _countof(buf), sel ? L"> %d Hz" : L"   %d Hz", REFRESH_RATES[i]);
            if (act) wcscat(buf, L"  [ACTIVE]");
            r.drawTextLeft(buf, 50*sc, contentY+i*optStep, WINDOW_WIDTH-50*sc, optStep, ff, 16*sc, FontStyleRegular, col);
        }
    } else {
        const wchar_t* optNames[] = { L"Anti-Alias", L"Performance Mode", L"Render Backend" };
        float optStep = 32 * sc;
        for (int i = 0; i < 2; i++) {
            Color col = (i == settingsSelection) ? Color(255,255,255,140) : Color(255,185,185,185);
            bool on = (i == 0) ? antiAlias : perfMode;
            wchar_t buf[80];
            swprintf_s(buf, _countof(buf), (i==settingsSelection) ? L"> %-20s  %s" : L"   %-20s  %s",
                       optNames[i], on ? L"ON" : L"OFF");
            r.drawTextLeft(buf, 50*sc, contentY+i*optStep, WINDOW_WIDTH-50*sc, optStep, ff, 16*sc, FontStyleRegular, col);
        }
        if (d2dAvailable) {
            int i = 2; Color col = (i == settingsSelection) ? Color(255,255,255,140) : Color(255,185,185,185);
            wchar_t buf[80];
            swprintf_s(buf, _countof(buf), (i==settingsSelection) ? L"> %-20s  %s" : L"   %-20s  %s",
                       optNames[i], useDirect2D ? L"Direct2D" : L"GDI+");
            r.drawTextLeft(buf, 50*sc, contentY+i*optStep, WINDOW_WIDTH-50*sc, optStep, ff, 16*sc, FontStyleRegular, col);
        }
    }

    float hintY = contentY + 6 * 32 * sc + 10 * sc;
    if (hintY > WINDOW_HEIGHT - 26 * sc) hintY = WINDOW_HEIGHT - 26 * sc;
    r.drawTextCentered(settingsPage == 2 ?
        L"Arrows=Select  Enter=Toggle  Tab=Next Page  ESC=Back" :
        L"Arrows=Select  Enter=Apply  Tab=Next Page  ESC=Back",
        0, hintY, (float)WINDOW_WIDTH, 20*sc, ff, 11*sc, FontStyleRegular, Color(255,110,110,110));
}

// ========== 暂停 ==========

void Game::renderPauseOverlay(Renderer& r) {
    r.fillRect(0,0,(float)WINDOW_WIDTH,(float)WINDOW_HEIGHT, Color(160,0,0,0));
    float sc=(std::min)(WINDOW_HEIGHT/480.0f, 2.2f); const wchar_t* ff=L"Arial";
    renderBackBtn(r, sc, L"RESUME");
    r.drawTextCentered(L"PAUSED",0,120*sc,(float)WINDOW_WIDTH,60*sc,ff,30*sc,FontStyleBold,Color(255,255,220,0));
    r.drawTextCentered(L"P = Resume    ESC = Menu",0,220*sc,(float)WINDOW_WIDTH,40*sc,ff,15*sc,FontStyleRegular,Color(255,185,185,185));
}

// ========== 结束/胜利 ==========

void Game::renderGameOver(Renderer& r) {
    r.fillRect(0,0,(float)WINDOW_WIDTH,(float)WINDOW_HEIGHT, Color(190,2,2,18));
    float sc=(std::min)(WINDOW_HEIGHT/480.0f, 2.2f);
    renderBackBtn(r, sc); bool vic=(state==GameState::VICTORY);
    const wchar_t* ff=L"Arial";
    r.drawTextCentered(vic?L"VICTORY":L"GAME OVER",0,40*sc,(float)WINDOW_WIDTH,60*sc,ff,34*sc,FontStyleBold,
                       vic?Color(255,255,220,0):Color(255,255,45,45));
    wchar_t buf[128]; float y=120*sc, step=30*sc;
    swprintf_s(buf,_countof(buf),L"FINAL SCORE: %d",score); r.drawTextCentered(buf,0,y,(float)WINDOW_WIDTH,step,ff,17*sc,FontStyleRegular,Color::White);
    y+=step; swprintf_s(buf,_countof(buf),L"LEVEL: %d",currentLevel); r.drawTextCentered(buf,0,y,(float)WINDOW_WIDTH,step,ff,17*sc,FontStyleRegular,Color(255,185,185,185));
    y+=step; swprintf_s(buf,_countof(buf),L"HOSTILES: %d",enemiesKilledThisLevel); r.drawTextCentered(buf,0,y,(float)WINDOW_WIDTH,step,ff,17*sc,FontStyleRegular,Color(255,185,185,185));
    y+=step*1.5f;
    if (isHighScore()) {
        r.drawTextCentered(L"NEW HIGH SCORE",0,y,(float)WINDOW_WIDTH,step,ff,17*sc,FontStyleRegular,Color(255,255,220,0));
        y+=step;
        r.drawTextCentered(L"Press Enter to Record",0,y,(float)WINDOW_WIDTH,step,ff,17*sc,FontStyleRegular,Color(255,255,220,0));
        y+=step;
    }
    r.drawTextCentered(L"Press Enter to Continue",0,y,(float)WINDOW_WIDTH,step,ff,17*sc,FontStyleRegular,Color(255,185,185,185));
}

// ========== 排行榜 ==========

void Game::renderLeaderboard(Renderer& r) {
    float sc=(std::min)(WINDOW_HEIGHT/480.0f, 2.2f);
    const wchar_t* ffA=L"Arial", *ffC=L"Consolas";
    renderBackBtn(r, sc);
    r.drawTextCentered(L"LEADERBOARD",0,8*sc,(float)WINDOW_WIDTH,34*sc,ffA,26*sc,FontStyleBold,Color(255,255,220,0));
    wchar_t buf[128]; swprintf_s(buf,_countof(buf),L"RANK NAME      SCORE  LV DATE");
    r.drawTextLeft(buf,25,48*sc,WINDOW_WIDTH-25.0f,17*sc,ffC,13*sc,FontStyleRegular,Color(255,140,140,140));
    r.drawLine(25,65*sc,WINDOW_WIDTH-25.0f,65*sc,Color(255,70,70,70),1);

    float rowH=20*sc, startY=70*sc;
    for (size_t i=0;i<leaderboard.size();i++) {
        auto& e=leaderboard[i];
        Color col=(i==0)?Color(255,255,215,0):(i==1)?Color(255,192,192,192):(i==2)?Color(255,205,127,50):Color(255,190,190,190);
        wchar_t n[32]={}; mbstowcs(n,e.name,NAME_MAX_LEN);
        swprintf_s(buf,_countof(buf),L"%-4d %-9s %-7d %-3d %hs",(int)(i+1),n,e.score,e.level,e.date);
        r.drawTextLeft(buf,25,startY+i*rowH,WINDOW_WIDTH-25.0f,rowH,ffC,13*sc,FontStyleRegular,col);
    }
    if (leaderboard.empty()) { r.drawTextCentered(L"No records. Go play!",0,80*sc,(float)WINDOW_WIDTH,40*sc,ffC,13*sc,FontStyleRegular,Color(255,140,140,140)); }
    r.drawTextCentered(L"ESC = Back",0,WINDOW_HEIGHT-26*sc,(float)WINDOW_WIDTH,20*sc,ffC,13*sc,FontStyleRegular,Color(255,110,110,110));
}

// ========== 名字输入 ==========

void Game::renderNameEntry(Renderer& r) {
    r.fillRect(0,0,(float)WINDOW_WIDTH,(float)WINDOW_HEIGHT, Color(200,3,3,22));
    float sc=(std::min)(WINDOW_HEIGHT/480.0f, 2.2f);
    renderBackBtn(r, sc);
    const wchar_t* ffA=L"Arial", *ffC=L"Consolas";
    r.drawTextCentered(L"NEW HIGH SCORE",0,50*sc,(float)WINDOW_WIDTH,40*sc,ffA,26*sc,FontStyleBold,Color(255,255,220,0));
    wchar_t buf[128]; swprintf_s(buf,_countof(buf),L"Score: %d   Level: %d",score,currentLevel);
    r.drawTextCentered(buf,0,110*sc,(float)WINDOW_WIDTH,30*sc,ffC,15*sc,FontStyleRegular,Color::White);
    r.drawTextCentered(L"Enter name (3 letters):",0,165*sc,(float)WINDOW_WIDTH,30*sc,ffC,15*sc,FontStyleRegular,Color(255,185,185,185));
    wchar_t nd[32]={}; for(int i=0;i<nameLength;i++) nd[i]=(wchar_t)nameBuffer[i]; wcscat(nd,L"_");
    r.drawTextCentered(nd,0,210*sc,(float)WINDOW_WIDTH,50*sc,ffC,32*sc,FontStyleBold,Color(255,0,255,255));
    r.drawTextCentered(L"A-Z=Input  Backspace=Del  Enter=OK  ESC=Skip",0,300*sc,(float)WINDOW_WIDTH,35*sc,ffC,15*sc,FontStyleRegular,Color(255,110,110,110));
}

// ========== 关卡过渡 ==========

void Game::renderLevelTransition(Renderer& r) {
    float prog=1.0f-(float)stateTimer/LEVEL_TRANSITION_FRAMES;
    float sz=1.0f+sinf(prog*3.14159f)*0.3f;
    int fs=(int)(38*sz), al=(int)(255*(1.0f-fabsf(prog-0.5f)*2.0f));
    al=(std::max)(60,(std::min)(255,al));
    wchar_t buf[64]; swprintf_s(buf,_countof(buf),L"LEVEL %d",currentLevel);
    r.drawTextCentered(buf, 0, 140, (float)WINDOW_WIDTH, 100.0f, L"Arial", (float)fs, FontStyleBold, Color((BYTE)al,(BYTE)(al*200/255),0));
}

// ========== 帮助画面 ==========

void Game::renderHelp(Renderer& r) {
    r.fillRect(0,0,(float)WINDOW_WIDTH,(float)WINDOW_HEIGHT, Color(200,4,4,24));
    float sc=(std::min)(WINDOW_HEIGHT/480.0f, 2.2f); renderBackBtn(r, sc, L"CLOSE");
    const wchar_t* ffA = L"Arial", *ffC = L"Consolas";
    r.drawTextCentered(L"CONTROLS  [H/ESC to close]",0,8*sc,(float)WINDOW_WIDTH,32*sc,ffA,24*sc,FontStyleBold,Color(255,255,220,0));

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
        r.drawTextLeft(binds[i].k,c1,y,c2-c1-6*sc,rh,ffC,12*sc,FontStyleBold,Color::White);
        r.drawTextLeft(binds[i].d,c2,y,(float)WINDOW_WIDTH-c2-20*sc,rh,ffA,11*sc,FontStyleRegular,Color(255,180,180,180));
    }

    float iy=sy+nb*rh+8*sc;
    r.drawTextCentered(L"ITEMS",0,iy,(float)WINDOW_WIDTH,32*sc,ffA,24*sc,FontStyleBold,Color(255,255,220,0));
    struct { const wchar_t* i; const wchar_t* d; } itms[]={
        {L"[F] Orange",L"Firepower +1 (max 5)"},{L"[H] Red",L"+1 Life (max 5)"},
        {L"[B] Orange",L"+1 Bomb (max 3)"},{L"[S] Blue",L"Shield 10s"},
        {L"[R] Green",L"Fire Rate Up (8s)"},
    };
    const int ni=5; iy+=28*sc;
    for (int i=0;i<ni;i++) { float y=iy+i*rh;
        r.drawTextLeft(itms[i].i,c1,y,c2-c1-6*sc,rh,ffC,12*sc,FontStyleBold,Color::White);
        r.drawTextLeft(itms[i].d,c2,y,(float)WINDOW_WIDTH-c2-20*sc,rh,ffA,11*sc,FontStyleRegular,Color(255,180,180,180));
    }

    r.drawTextCentered(L"Press H or ESC to return to game",0,WINDOW_HEIGHT-16*sc,(float)WINDOW_WIDTH,14*sc,ffA,11*sc,FontStyleRegular,Color(255,130,130,130));
}

// ── 通用返回按钮（右上角） ──
static void renderBackBtn(Renderer& r, float sc, const wchar_t* label) {
    float btnW = (std::max)(70.0f, 80 * sc), btnH = (std::max)(20.0f, 24 * sc);
    float bx = WINDOW_WIDTH - btnW - (std::max)(8.0f, 12 * sc), by = (std::max)(4.0f, 8 * sc);
    r.fillRect(bx, by, btnW, btnH, Color(220, 50, 50, 80));
    r.drawRect(bx, by, btnW, btnH, Color(255, 200, 200, 200), 1.5f);
    r.drawTextCentered(label, bx, by, btnW, btnH, L"Arial", (std::max)(10.0f, 12 * sc), FontStyleBold, Color(255, 255, 255, 255));
}

static bool clickBackBtn(float mx, float my, float sc) {
    float btnW = (std::max)(70.0f, 80 * sc), btnH = (std::max)(20.0f, 24 * sc);
    float bx = WINDOW_WIDTH - btnW - (std::max)(8.0f, 12 * sc), by = (std::max)(4.0f, 8 * sc);
    return clickIn(mx, my, bx, by, btnW, btnH);
}

// ========== 皮肤选择 ==========

void Game::renderSkinSelect(Renderer& r) {
    float sc = (std::min)(WINDOW_HEIGHT / 480.0f, 2.2f);
    const wchar_t* ff = L"Arial";
    r.fillRect(0, 0, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT, Color(255, 3, 3, 20));
    renderBackBtn(r, sc);

    // 标题
    r.drawTextCentered(L"SELECT SKIN", 0, 20 * sc, (float)WINDOW_WIDTH, 40 * sc, ff, 28 * sc, FontStyleBold, Color(255, 255, 220, 0));

    // 皮肤名
    wchar_t nameBuf[64];
    swprintf_s(nameBuf, _countof(nameBuf), L"<  %s  >", SKINS[currentSkin].name);
    r.drawTextCentered(nameBuf, 0, 60 * sc, (float)WINDOW_WIDTH, 35 * sc, ff, 22 * sc, FontStyleBold, Color(255, 255, 255, 200));

    // ── 预览区：缩小版玩家战机 + 敌机 ──
    float prevCX = WINDOW_WIDTH / 2.0f, prevCY = WINDOW_HEIGHT / 2.0f - 20 * sc;
    float ps = sc * 0.7f;  // 预览缩放

    // 玩家（简化版）
    float pcx = prevCX - 50 * sc, pcy = prevCY;
    {
        PointF pBody[5] = {{pcx, pcy-20*ps},{pcx-16*ps, pcy+10*ps},{pcx-8*ps, pcy+20*ps},{pcx+8*ps, pcy+20*ps},{pcx+16*ps, pcy+10*ps}};
        r.fillPolygon(pBody, 5, Palette::PlayerBody);
        r.drawPolygon(pBody, 5, Palette::PlayerDark, 1.5f*ps);
        r.fillEllipse(pcx-5*ps, pcy-7*ps, 10*ps, 8*ps, Palette::PlayerCockpit);
        // 引擎火焰
        r.drawLine(pcx-4*ps, pcy+20*ps, pcx-4*ps, pcy+28*ps, Palette::PlayerEngine, 2*ps);
        r.drawLine(pcx+4*ps, pcy+20*ps, pcx+4*ps, pcy+28*ps, Palette::PlayerEngine, 2*ps);
        r.fillEllipse(pcx-12*ps, pcy-3*ps, 24*ps, 2*ps, Palette::PlayerWing);
    }
    r.drawTextCentered(L"PLAYER", pcx - 30 * ps, pcy + 32 * ps, 60 * ps, 14 * ps, ff, 10 * sc, FontStyleRegular, Color(255, 200, 200, 200));

    // 敌机（简化版 NormalEnemy）
    float ecx = prevCX + 50 * sc, ecy = prevCY;
    {
        PointF ePts[5] = {{ecx, ecy+20*ps},{ecx-16*ps, ecy-10*ps},{ecx-8*ps, ecy},{ecx+8*ps, ecy},{ecx+16*ps, ecy-10*ps}};
        r.fillPolygon(ePts, 5, Palette::EnemyNormal);
        r.drawPolygon(ePts, 5, Palette::EnemyNormalDark, 1.5f*ps);
        r.fillEllipse(ecx-3*ps, ecy+2*ps, 6*ps, 5*ps, Color(255, 255, 200, 200));
    }
    r.drawTextCentered(L"ENEMY", ecx - 30 * ps, ecy + 32 * ps, 60 * ps, 14 * ps, ff, 10 * sc, FontStyleRegular, Color(255, 200, 200, 200));

    // 子弹预览
    float bcy = prevCY + 60 * ps;
    r.fillEllipse(pcx - 10 * ps, bcy, 4*ps, 12*ps, Palette::BulletPlayer);
    r.drawTextCentered(L"BULLET", pcx - 30 * ps, bcy + 16 * ps, 60 * ps, 14 * ps, ff, 10 * sc, FontStyleRegular, Color(255, 200, 200, 200));
    r.fillEllipse(ecx - 10 * ps, bcy, 8*ps, 8*ps, Palette::BulletEnemy);

    // ── 底部按钮 ──
    float btnY = WINDOW_HEIGHT / 2.0f + 80 * sc;
    float btnW = 120 * sc, btnH = 35 * sc;
    r.fillRect(WINDOW_WIDTH / 2.0f - btnW - 20 * sc, btnY, btnW, btnH, Color(255, 60, 60, 80));
    r.drawRect(WINDOW_WIDTH / 2.0f - btnW - 20 * sc, btnY, btnW, btnH, Color(255, 150, 150, 150), 1);
    r.drawTextCentered(L"< PREV", WINDOW_WIDTH / 2.0f - btnW - 20 * sc, btnY, btnW, btnH, ff, 14 * sc, FontStyleBold, Color(255, 255, 255, 200));

    r.fillRect(WINDOW_WIDTH / 2.0f + 20 * sc, btnY, btnW, btnH, Color(255, 60, 80, 60));
    r.drawRect(WINDOW_WIDTH / 2.0f + 20 * sc, btnY, btnW, btnH, Color(255, 150, 150, 150), 1);
    r.drawTextCentered(L"NEXT >", WINDOW_WIDTH / 2.0f + 20 * sc, btnY, btnW, btnH, ff, 14 * sc, FontStyleBold, Color(255, 255, 255, 200));

    // 提示
    r.drawTextCentered(L"Left/Right = Switch  Enter/ESC = Confirm  Click buttons above",
                       0, btnY + btnH + 12 * sc, (float)WINDOW_WIDTH, 18 * sc, ff, 10 * sc, FontStyleRegular, Color(255, 140, 140, 140));
}

// ========== 各实体渲染委托 ==========

void Game::renderEnemies(Renderer& r) { for (auto* e : enemies) if (e->alive) e->render(r); }
void Game::renderBullets(Renderer& r) { for (auto* b : playerBullets) if (b->alive) b->render(r); for (auto* b : enemyBullets) if (b->alive) b->render(r); }
void Game::renderItems(Renderer& r)   { for (auto* it : items) if (it->alive) it->render(r); }

// ========== 发光辅助 ==========

void Game::drawGlow(Renderer& r, float cx, float cy, float radius, Color c, int layers) {
    for (int i=layers-1;i>=0;i--) {
        float a=1.0f-(float)i/layers*0.7f;
        float lw = 1+i*0.8f;
        Color glowC((BYTE)(c.GetA()*a),c.GetR(),c.GetG(),c.GetB());
        float rr = radius+i*2;
        r.drawEllipse(cx-rr, cy-rr, rr*2, rr*2, glowC, lw);
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
