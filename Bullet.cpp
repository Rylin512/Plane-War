#include "Bullet.h"
#include "Renderer.h"
using namespace Gdiplus;

Bullet::Bullet(float x, float y, int w, int h, BulletOwner _o, float _sy)
    : GameObject(x,y,w,h), owner(_o), speedY(_sy), damage(1) {}

void Bullet::update() { y += speedY; }

void Bullet::render(Renderer& r) const {
    float sc = GAME_SCALE, cx = centerX(), cy = centerY();
    if (owner == BulletOwner::PLAYER) {
        // 激光弹 — 宽高已含 GAME_SCALE，光晕偏移单独缩放
        r.fillEllipse(x-2*sc, y-2*sc, width+4*sc, height+4*sc, Palette::BulletPlayer);
        r.fillEllipse(x, y, (float)width, (float)height, Palette::BulletPlayerCore);
        r.fillRect(cx-1*sc, y, 2*sc, (float)height, Palette::BulletPlayerCore);
    } else {
        // 火球弹 — 炽热圆形
        r.fillEllipse(x-2*sc, y-2*sc, width+4*sc, height+4*sc, Color(255,255,80,30));
        r.fillEllipse(x, y, (float)width, (float)height, Palette::BulletEnemy);
        r.fillEllipse(cx-1.5f*sc, cy-1.5f*sc, 3*sc, 3*sc, Color(255,255,200,150));
    }
}

PlayerBullet::PlayerBullet(float x, float y)
    : Bullet(x, y, (int)(BULLET_PLAYER_WIDTH * GAME_SCALE), (int)(BULLET_PLAYER_HEIGHT * GAME_SCALE),
             BulletOwner::PLAYER, -BULLET_PLAYER_SPEED * GAME_SCALE) {}

void PlayerBullet::reset(float nx, float ny) {
    x = nx; y = ny;
    width  = (int)(BULLET_PLAYER_WIDTH  * GAME_SCALE);
    height = (int)(BULLET_PLAYER_HEIGHT * GAME_SCALE);
    speedY = -BULLET_PLAYER_SPEED * GAME_SCALE;
    alive = true; damage = 1;
}

EnemyBullet::EnemyBullet(float x, float y, float ax)
    : Bullet(x, y, (int)(BULLET_ENEMY_WIDTH * GAME_SCALE), (int)(BULLET_ENEMY_HEIGHT * GAME_SCALE),
             BulletOwner::ENEMY, BULLET_ENEMY_SPEED * GAME_SCALE), speedX(ax * GAME_SCALE) {}

void EnemyBullet::reset(float nx, float ny, float nAngleX) {
    x = nx; y = ny;
    width  = (int)(BULLET_ENEMY_WIDTH  * GAME_SCALE);
    height = (int)(BULLET_ENEMY_HEIGHT * GAME_SCALE);
    speedX = nAngleX * GAME_SCALE;
    speedY = BULLET_ENEMY_SPEED * GAME_SCALE;
    alive = true; damage = 1;
}

void EnemyBullet::update() { x+=speedX; y+=speedY; }
void EnemyBullet::render(Renderer& r) const {
    float sc = GAME_SCALE, cx=centerX(), cy=centerY();
    r.fillEllipse(x-2*sc, y-2*sc, width+4*sc, height+4*sc, Color(255,255,80,30));
    r.fillEllipse(x, y, (float)width, (float)height, Palette::BulletEnemy);
    r.fillEllipse(cx-1.5f*sc, cy-1.5f*sc, 3*sc, 3*sc, Palette::BulletEnemyCore);
}
