#include "Bullet.h"
using namespace Gdiplus;

Bullet::Bullet(float x, float y, int w, int h, BulletOwner _o, float _sy)
    : GameObject(x,y,w,h), owner(_o), speedY(_sy), damage(1) {}

void Bullet::update() { y += speedY; }

void Bullet::render(Graphics& g) const {
    float sc = GAME_SCALE, cx = centerX(), cy = centerY();
    if (owner == BulletOwner::PLAYER) {
        // 激光弹 — 细长发光
        SolidBrush gb(Color(255,255,255,100)); g.FillEllipse(&gb, x-2*sc, y-2*sc, (width+4)*sc, (height+4)*sc);
        SolidBrush bb(Color(255,255,255,200)); g.FillEllipse(&bb, x, y, width*sc, height*sc);
        SolidBrush cb(Color(255,255,255,255)); g.FillRectangle(&cb, cx-1*sc, y, 2*sc, height*sc);
    } else {
        // 火球弹 — 炽热圆形
        SolidBrush gb(Color(255,255,80,30)); g.FillEllipse(&gb, x-2*sc, y-2*sc, (width+4)*sc, (height+4)*sc);
        SolidBrush eb(Palette::BulletEnemy); g.FillEllipse(&eb, x, y, width*sc, height*sc);
        SolidBrush cb(Color(255,255,200,150)); g.FillEllipse(&cb, cx-1.5f*sc, cy-1.5f*sc, 3*sc, 3*sc);
    }
}

PlayerBullet::PlayerBullet(float x, float y)
    : Bullet(x,y,BULLET_PLAYER_WIDTH,BULLET_PLAYER_HEIGHT,BulletOwner::PLAYER,-BULLET_PLAYER_SPEED*GAME_SCALE) {}

EnemyBullet::EnemyBullet(float x, float y, float ax)
    : Bullet(x,y,BULLET_ENEMY_WIDTH,BULLET_ENEMY_HEIGHT,BulletOwner::ENEMY,BULLET_ENEMY_SPEED*GAME_SCALE), speedX(ax*GAME_SCALE) {}

void EnemyBullet::update() { x+=speedX; y+=speedY; }
void EnemyBullet::render(Graphics& g) const {
    float sc = GAME_SCALE, cx=centerX(), cy=centerY();
    SolidBrush gb(Color(255,255,80,30)); g.FillEllipse(&gb, x-2*sc, y-2*sc, (width+4)*sc, (height+4)*sc);
    SolidBrush eb(Palette::BulletEnemy); g.FillEllipse(&eb, x, y, width*sc, height*sc);
    SolidBrush cb(Palette::BulletEnemyCore); g.FillEllipse(&cb, cx-1.5f*sc, cy-1.5f*sc, 3*sc, 3*sc);
}
