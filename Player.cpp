#include "Player.h"
#include "Game.h"
#include <cmath>

using namespace Gdiplus;

// ============================================================
// Player.cpp — 增强版程序化精灵：细节机身 + 多层引擎 + 速度特效
// ============================================================

Player::Player()
    : GameObject(WINDOW_WIDTH/2.0f-(int)(PLAYER_WIDTH*GAME_SCALE)/2.0f,
                 WINDOW_HEIGHT-80.0f*GAME_SCALE,
                 (int)(PLAYER_WIDTH*GAME_SCALE), (int)(PLAYER_HEIGHT*GAME_SCALE))
    , lives(PLAYER_INIT_LIVES), firepowerLevel(1), firepowerTimer(0)
    , invincibleTimer(0), shootCooldown(0), bombCount(0)
    , shieldActive(false), shieldTimer(0), fireRateBoost(false), fireRateTimer(0)
    , blinkCounter(0), speedX(0), speedY(0)
    , fastMode(false), slowMode(false), currentSpeedMult(1.0f)
{}

void Player::setFastMode(bool on)  { fastMode = on; updateSpeedMult(); }
void Player::setSlowMode(bool on)  { slowMode = on; updateSpeedMult(); }

void Player::updateSpeedMult() {
    if (fastMode && !slowMode)       currentSpeedMult = PLAYER_FAST_MULT;
    else if (slowMode && !fastMode)  currentSpeedMult = PLAYER_SLOW_MULT;
    else                             currentSpeedMult = 1.0f;
}

float Player::getCurrentSpeed() const { return PLAYER_BASE_SPEED * currentSpeedMult; }

void Player::update() {
    x += speedX * currentSpeedMult;
    y += speedY * currentSpeedMult;
    clampToScreen();
    if (shootCooldown > 0)    shootCooldown--;
    if (invincibleTimer > 0)  { invincibleTimer--; blinkCounter++; }
    if (shieldActive)         { if (--shieldTimer <= 0) shieldActive = false; }
    if (firepowerTimer > 0)   { if (--firepowerTimer <= 0 && firepowerLevel > 1) firepowerLevel = 1; }
    if (fireRateBoost)        { if (--fireRateTimer <= 0) fireRateBoost = false; }
}

void Player::move(float dx, float dy) { x+=dx; y+=dy; clampToScreen(); }
void Player::clampToScreen() {
    x = clamp(x, 0.0f, (float)(WINDOW_WIDTH - width));
    y = clamp(y, 0.0f, (float)(WINDOW_HEIGHT - height));
}

bool Player::canShoot() const        { return shootCooldown <= 0; }
void Player::resetShootCooldown()    { shootCooldown = fireRateBoost ? PLAYER_FAST_SHOOT : PLAYER_SHOOT_COOLDOWN; }
void Player::takeDamage() {
    if (isInvincible()) return;
    if (shieldActive) { shieldActive = false; shieldTimer = 0; return; }
    lives--; invincibleTimer = PLAYER_INVINCIBLE_FRAMES; blinkCounter = 0;
    if (lives < 0) lives = 0;
}
bool Player::isInvincible() const    { return invincibleTimer > 0; }
void Player::addLife()               { if (lives < PLAYER_MAX_LIVES) lives++; }
void Player::addBomb()               { if (bombCount < PLAYER_MAX_BOMBS) bombCount++; }
void Player::activateShield()        { shieldActive = true; shieldTimer = SHIELD_DURATION; }
void Player::increaseFirepower()     { if (firepowerLevel < PLAYER_MAX_FIREPOWER) firepowerLevel++; firepowerTimer = FIREPOWER_DURATION; }
void Player::boostFireRate()         { fireRateBoost = true; fireRateTimer = FIRERATE_DURATION; }

// ========================================================================
//  GDI+ 渲染 — 像素风科幻战机
// ========================================================================

void Player::render(Graphics& g) const {
    if (isInvincible() && (blinkCounter/6)%2 == 0) return;

    float sc = GAME_SCALE;
    float cx = centerX(), cy = centerY();
    float w = width, h = height;

    // ── 速度光晕 ──
    {
        Color glowC = fastMode ? Color(255,0,255,80) : slowMode ? Color(255,80,180,255) : Color(255,0,100,200);
        for (int i=2;i>=0;i--) {
            Pen gp(glowC, 1.0f+i*1.2f*sc);
            float r = (22+i*5)*sc;
            g.DrawEllipse(&gp, cx-r, cy-r, r*2, r*2);
        }
    }

    // ── 护盾 ──
    if (shieldActive) {
        for (int i=3;i>=0;i--) {
            Pen p(Color((BYTE)(200-i*40), 0, 180, 255), 2.0f+i*2.0f*sc);
            float r = (30+i*5)*sc;
            g.DrawEllipse(&p, cx-r, cy-r, r*2, r*2);
        }
    }

    // ── 引擎火焰 ──
    int flick = 4 + blinkCounter%6;
    // 外焰
    {
        Pen f1(Color(255, 255, 80+rand()%50, 10), 3*sc);
        g.DrawLine(&f1, cx-5*sc, cy+h/2, cx-5*sc, cy+h/2+(9+flick)*sc);
        g.DrawLine(&f1, cx+5*sc, cy+h/2, cx+5*sc, cy+h/2+(9+flick)*sc);
        // 中焰
        Pen f2(Color(255, 255, 200, 30), 2*sc);
        int fh = 5 + blinkCounter%4;
        g.DrawLine(&f2, cx-5*sc, cy+h/2, cx-5*sc, cy+h/2+fh*sc);
        g.DrawLine(&f2, cx+5*sc, cy+h/2, cx+5*sc, cy+h/2+fh*sc);
        // 焰心
        Pen f3(Color(240, 255, 255, 200), 1*sc);
        int f3h = 2 + blinkCounter%3;
        g.DrawLine(&f3, cx-5*sc, cy+h/2, cx-5*sc, cy+h/2+f3h*sc);
        g.DrawLine(&f3, cx+5*sc, cy+h/2, cx+5*sc, cy+h/2+f3h*sc);
    }

    // ── 机身主体（多层多边形） ──
    // 阴影/暗面
    PointF shadow[6] = {{cx+2*sc,cy-h/2+2*sc},{cx-6*sc,cy+h/2-6*sc},{cx-2*sc,cy+h/2+2*sc},{cx+6*sc,cy+h/2+2*sc},{cx+10*sc,cy+h/2-6*sc},{cx+2*sc,cy-h/2+2*sc}};
    SolidBrush sdBr(Color(255,0,60,140)); g.FillPolygon(&sdBr, shadow, 6);

    // 主体亮色
    PointF body[6] = {{cx,cy-h/2},{cx-8*sc,cy+h/2-8*sc},{cx-4*sc,cy+h/2},{cx+4*sc,cy+h/2},{cx+8*sc,cy+h/2-8*sc}};
    SolidBrush bdBr(Color(255,0,180,255)); g.FillPolygon(&bdBr, body, 5);
    Pen bdPn(Color(255,0,100,200), 1.5f*sc); g.DrawPolygon(&bdPn, body, 5);

    // 机身高光条纹
    Pen hlPn(Color(255,100,230,255), 1*sc);
    g.DrawLine(&hlPn, cx-2*sc, cy-h/2+6*sc, cx-1*sc, cy+h/2-6*sc);
    g.DrawLine(&hlPn, cx+2*sc, cy-h/2+6*sc, cx+1*sc, cy+h/2-6*sc);

    // ── 主翼 ──
    PointF lwing[4] = {{cx-20*sc,cy-2*sc},{cx-8*sc,cy-2*sc},{cx-8*sc,cy-8*sc},{cx-22*sc,cy-8*sc}};
    PointF rwing[4] = {{cx+20*sc,cy-2*sc},{cx+8*sc,cy-2*sc},{cx+8*sc,cy-8*sc},{cx+22*sc,cy-8*sc}};
    SolidBrush wBr(Color(255,150,190,220));
    g.FillPolygon(&wBr, lwing, 4); g.FillPolygon(&wBr, rwing, 4);
    Pen wPn(Color(255,100,150,190), 1.5f*sc);
    g.DrawPolygon(&wPn, lwing, 4); g.DrawPolygon(&wPn, rwing, 4);
    // 翼尖导弹
    Pen msPn(Color(255,255,220,80), 1.5f*sc);
    g.DrawLine(&msPn, cx-21*sc, cy-5*sc, cx-21*sc, cy-12*sc);
    g.DrawLine(&msPn, cx+21*sc, cy-5*sc, cx+21*sc, cy-12*sc);

    // ── 尾翼 ──
    Pen tlPn(Color(255,140,180,210), 2.5f*sc);
    g.DrawLine(&tlPn, cx-14*sc, cy+h/2-4*sc, cx+14*sc, cy+h/2-4*sc);

    // ── 驾驶舱 ──
    SolidBrush cpBg(Color(255,0,60,150)); g.FillEllipse(&cpBg, cx-7*sc, cy-9*sc, 14*sc, 12*sc);
    SolidBrush cpBr(Color(255,0,230,255)); g.FillEllipse(&cpBr, cx-5*sc, cy-8*sc, 10*sc, 9*sc);
    // 舱内高光
    SolidBrush cpHi(Color(150,255,255,255)); g.FillEllipse(&cpHi, cx-3*sc, cy-8*sc, 4*sc, 3*sc);
}
