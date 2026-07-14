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

void Player::render(Renderer& r) const {
    if (isInvincible() && (blinkCounter/6)%2 == 0) return;

    float sc = GAME_SCALE;
    float cx = centerX(), cy = centerY();
    float h = height;

    // ── 速度光晕 ──
    {
        Color glowC = fastMode ? Palette::PlayerFastGlow : slowMode ? Palette::PlayerSlowGlow : Palette::PlayerGlow;
        for (int i=2;i>=0;i--) {
            float radius = (22+i*5)*sc;
            r.drawEllipse(cx-radius, cy-radius, radius*2, radius*2, glowC, 1.0f+i*1.2f*sc);
        }
    }

    // ── 护盾 ──
    if (shieldActive) {
        Color shC = Palette::PlayerGlow;
        for (int i=3;i>=0;i--) {
            float radius = (30+i*5)*sc;
            r.drawEllipse(cx-radius, cy-radius, radius*2, radius*2, Color((BYTE)(200-i*40), shC.GetR(), shC.GetG(), shC.GetB()), 2.0f+i*2.0f*sc);
        }
    }

    // ── 引擎火焰 ──
    int flick = 4 + blinkCounter%6;
    Color eng = Palette::PlayerEngine;
    {
        Color f1c(eng.GetA(), eng.GetR(), (BYTE)(eng.GetG()*80/255+rand()%50), eng.GetB()/2);
        r.drawLine(cx-5*sc, cy+h/2, cx-5*sc, cy+h/2+(9+flick)*sc, f1c, 3*sc);
        r.drawLine(cx+5*sc, cy+h/2, cx+5*sc, cy+h/2+(9+flick)*sc, f1c, 3*sc);
        int fh = 5 + blinkCounter%4;
        Color f2c(eng.GetA()/2, eng.GetR(), eng.GetG(), eng.GetB());
        r.drawLine(cx-5*sc, cy+h/2, cx-5*sc, cy+h/2+fh*sc, f2c, 2*sc);
        r.drawLine(cx+5*sc, cy+h/2, cx+5*sc, cy+h/2+fh*sc, f2c, 2*sc);
        int f3h = 2 + blinkCounter%3;
        Color f3c(240, 255, 255, 200);
        r.drawLine(cx-5*sc, cy+h/2, cx-5*sc, cy+h/2+f3h*sc, f3c, 1*sc);
        r.drawLine(cx+5*sc, cy+h/2, cx+5*sc, cy+h/2+f3h*sc, f3c, 1*sc);
    }

    // ── 机身主体 ──
    Color pBody = Palette::PlayerBody, pDark = Palette::PlayerDark, pWing = Palette::PlayerWing;
    PointF shadow[6] = {{cx+2*sc,cy-h/2+2*sc},{cx-6*sc,cy+h/2-6*sc},{cx-2*sc,cy+h/2+2*sc},{cx+6*sc,cy+h/2+2*sc},{cx+10*sc,cy+h/2-6*sc},{cx+2*sc,cy-h/2+2*sc}};
    r.fillPolygon(shadow, 6, pDark);

    PointF body[6] = {{cx,cy-h/2},{cx-8*sc,cy+h/2-8*sc},{cx-4*sc,cy+h/2},{cx+4*sc,cy+h/2},{cx+8*sc,cy+h/2-8*sc}};
    r.fillPolygon(body, 5, pBody);
    r.drawPolygon(body, 5, pDark, 1.5f*sc);

    // 高光条纹
    r.drawLine(cx-2*sc, cy-h/2+6*sc, cx-1*sc, cy+h/2-6*sc, pWing, 1*sc);
    r.drawLine(cx+2*sc, cy-h/2+6*sc, cx+1*sc, cy+h/2-6*sc, pWing, 1*sc);

    // ── 主翼 ──
    PointF lwing[4] = {{cx-20*sc,cy-2*sc},{cx-8*sc,cy-2*sc},{cx-8*sc,cy-8*sc},{cx-22*sc,cy-8*sc}};
    PointF rwing[4] = {{cx+20*sc,cy-2*sc},{cx+8*sc,cy-2*sc},{cx+8*sc,cy-8*sc},{cx+22*sc,cy-8*sc}};
    r.fillPolygon(lwing, 4, pWing);
    r.fillPolygon(rwing, 4, pWing);
    r.drawPolygon(lwing, 4, pDark, 1.5f*sc);
    r.drawPolygon(rwing, 4, pDark, 1.5f*sc);
    // 翼尖导弹
    r.drawLine(cx-21*sc, cy-5*sc, cx-21*sc, cy-12*sc, pWing, 1.5f*sc);
    r.drawLine(cx+21*sc, cy-5*sc, cx+21*sc, cy-12*sc, pWing, 1.5f*sc);

    // ── 尾翼 ──
    r.drawLine(cx-14*sc, cy+h/2-4*sc, cx+14*sc, cy+h/2-4*sc, pWing, 2.5f*sc);

    // ── 驾驶舱 ──
    Color pCockpit = Palette::PlayerCockpit;
    r.fillEllipse(cx-7*sc, cy-9*sc, 14*sc, 12*sc, Color(pCockpit.GetA(), pCockpit.GetR()/4, pCockpit.GetG()/4, pCockpit.GetB()/2));
    r.fillEllipse(cx-5*sc, cy-8*sc, 10*sc, 9*sc, pCockpit);
    r.fillEllipse(cx-3*sc, cy-8*sc, 4*sc, 3*sc, Color(150, 255, 255, 255));
}
