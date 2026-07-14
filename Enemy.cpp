#include "Enemy.h"
#include "Renderer.h"
#include <cmath>

using namespace Gdiplus;

Enemy::Enemy(float x, float y, int w, int h, EnemyType _type)
    : GameObject(x,y,w,h), type(_type), movePattern(MovePattern::STRAIGHT)
    , speed(1), hp(1), maxHp(1), scoreValue(10)
    , shootTimer(0), shootInterval(60)
    , sinePhase(0), sineAmplitude(0), animFrame(0) {}

void Enemy::update() {
    animFrame++;
    switch (movePattern) {
    case MovePattern::SINE:    x += sinf(animFrame*0.08f+sinePhase)*sineAmplitude; break;
    case MovePattern::ZIGZAG:  x += (animFrame%40 < 20 ? 1.0f : -1.0f) * sineAmplitude; break;
    case MovePattern::SPIRAL:  x += cosf(animFrame*0.06f+sinePhase)*(2.0f+animFrame*0.02f); break;
    case MovePattern::CIRCLE:  x += cosf(animFrame*0.05f+sinePhase)*sineAmplitude; y += sinf(animFrame*0.05f+sinePhase)*1.5f; break;
    default: break;
    }
    y += speed;
    if (shootTimer>0) shootTimer--;
}

void Enemy::render(Renderer& r) const {
    float sc = GAME_SCALE;
    float cx=centerX(), cy=centerY();

    switch (type) {
    case EnemyType::NORMAL: {
        Color nBody = Palette::EnemyNormal, nDark = Palette::EnemyNormalDark;
        for (int i=1;i>=0;i--) r.drawEllipse(cx-22*sc, cy-22*sc, 44*sc, 44*sc, Color(255,(BYTE)(nBody.GetR()*(80-i*30)/225),nBody.GetG()/5,nBody.GetB()/5), (float)(1+i));
        PointF pts[5]={{cx,cy+height/2.0f},{cx-width/2.0f,cy-height/2.0f},{cx-width/4.0f,cy-height/4.0f},{cx+width/4.0f,cy-height/4.0f},{cx+width/2.0f,cy-height/2.0f}};
        r.fillPolygon(pts, 5, nBody);
        r.drawPolygon(pts, 5, nDark, 1.5f*sc);
        r.drawLine(cx, cy-height/2.0f+4*sc, cx, cy+height/2.0f-4*sc, Color(255,nBody.GetR()*180/225,nBody.GetG()*50/38,nBody.GetB()*50/38), 1*sc);
        r.drawLine(cx-width/2.0f, cy-height/4.0f, cx-width/2.0f-8*sc, cy+4*sc, nDark, 2*sc);
        r.drawLine(cx+width/2.0f, cy-height/4.0f, cx+width/2.0f+8*sc, cy+4*sc, nDark, 2*sc);
        r.fillEllipse(cx-width/2.0f-9*sc, cy+3*sc, 4*sc, 4*sc, Color(255,255,200,100));
        r.fillEllipse(cx+width/2.0f+5*sc, cy+3*sc, 4*sc, 4*sc, Color(255,255,200,100));
        r.fillEllipse(cx-3*sc, cy-2*sc, 6*sc, 5*sc, Color(255,255,200,200));
        r.fillEllipse(cx-1*sc, cy-1*sc, 2*sc, 2*sc, Color(255,255,80,80));
        break;
    }
    case EnemyType::FAST: {
        Color fBody = Palette::EnemyFast, fDark = Palette::EnemyFastDark;
        PointF pts[3]={{cx,cy+height/2.0f},{cx-width/2.0f,cy-height/2.0f},{cx+width/2.0f,cy-height/2.0f}};
        r.fillPolygon(pts, 3, fBody);
        r.drawPolygon(pts, 3, fDark, 1.5f*sc);
        r.drawLine(cx, cy-height/2.0f, cx, cy-height/2.0f-5*sc, Color(255,255,200,60), 1*sc);
        r.drawLine(cx-5*sc, cy-height/2.0f+3*sc, cx-6*sc, cy-height/2.0f-2*sc, Color(255,255,200,60), 1*sc);
        r.drawLine(cx+5*sc, cy-height/2.0f+3*sc, cx+6*sc, cy-height/2.0f-2*sc, Color(255,255,200,60), 1*sc);
        r.drawLine(cx, cy+height/2.0f-2*sc, cx, cy+height/2.0f+3*sc, Color(255,255,180,40), 1.5f*sc);
        break;
    }
    case EnemyType::SHOOTING: {
        Color sBody = Palette::EnemyShooting, sLit = Palette::EnemyShootingLit;
        for (int i=1;i>=0;i--) r.drawEllipse(cx-24*sc, cy-24*sc, 48*sc, 48*sc, Color(255,(BYTE)(sBody.GetR()*(60-i*20)/165),sBody.GetG()/2,sBody.GetB()*90/225), (float)(1+i));
        PointF hx[6]={{cx-12*sc,cy-15*sc},{cx+12*sc,cy-15*sc},{cx+17*sc,cy},{cx+12*sc,cy+15*sc},{cx-12*sc,cy+15*sc},{cx-17*sc,cy}};
        r.fillPolygon(hx, 6, sBody);
        r.drawPolygon(hx, 6, sLit, 2*sc);
        r.drawLine(cx-6*sc, cy+15*sc, cx-6*sc, cy+15*sc+8*sc, sLit, 2.5f*sc);
        r.drawLine(cx+6*sc, cy+15*sc, cx+6*sc, cy+15*sc+8*sc, sLit, 2.5f*sc);
        if (animFrame%20<5) {
            r.fillEllipse(cx-7*sc, cy+15*sc+7*sc, 3*sc, 3*sc, Color(255,255,200,255));
            r.fillEllipse(cx+5*sc, cy+15*sc+7*sc, 3*sc, 3*sc, Color(255,255,200,255));
        }
        r.drawEllipse(cx-10*sc, cy-8*sc, 20*sc, 16*sc, Color(255,(BYTE)(sLit.GetR()*200/210),sLit.GetG()*80/100,sLit.GetB()*240/255), 1*sc);
        break;
    }
    case EnemyType::BOSS: break;
    }
}

bool Enemy::shouldShoot(int fc) { return shootTimer<=0 && (fc%shootInterval==0); }
void Enemy::resetShootTimer()   { shootTimer = shootInterval; }

NormalEnemy::NormalEnemy(float sx)
    : Enemy(sx,-(float)ENEMY_NORMAL_HEIGHT*GAME_SCALE,(int)(ENEMY_NORMAL_WIDTH*GAME_SCALE),(int)(ENEMY_NORMAL_HEIGHT*GAME_SCALE),EnemyType::NORMAL)
    { speed=ENEMY_NORMAL_SPEED*GAME_SCALE; hp=1; maxHp=1; scoreValue=ENEMY_NORMAL_SCORE; }

FastEnemy::FastEnemy(float sx)
    : Enemy(sx,-(float)ENEMY_FAST_HEIGHT*GAME_SCALE,(int)(ENEMY_FAST_WIDTH*GAME_SCALE),(int)(ENEMY_FAST_HEIGHT*GAME_SCALE),EnemyType::FAST)
    { speed=ENEMY_FAST_SPEED*GAME_SCALE; hp=1; maxHp=1; scoreValue=ENEMY_FAST_SCORE; }

ShootingEnemy::ShootingEnemy(float sx)
    : Enemy(sx,-(float)ENEMY_SHOOTING_HEIGHT*GAME_SCALE,(int)(ENEMY_SHOOTING_WIDTH*GAME_SCALE),(int)(ENEMY_SHOOTING_HEIGHT*GAME_SCALE),EnemyType::SHOOTING)
    { speed=ENEMY_SHOOTING_SPEED*GAME_SCALE; hp=2; maxHp=2; scoreValue=ENEMY_SHOOTING_SCORE; shootInterval=70; shootTimer=30+rand()%40; }
bool ShootingEnemy::shouldShoot(int) { return shootTimer<=0; }
