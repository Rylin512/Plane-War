#include "Enemy.h"
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

void Enemy::render(Graphics& g) const {
    float sc = GAME_SCALE;
    float cx=centerX(), cy=centerY();

    switch (type) {
    case EnemyType::NORMAL: {
        // 异星战机 — 倒三角 + 触角引擎
        // 光晕
        for (int i=1;i>=0;i--) { Pen gp(Color(255,80-i*30,8,8),1+i); g.DrawEllipse(&gp,cx-22*sc,cy-22*sc,44*sc,44*sc); }
        // 机身
        PointF pts[5]={{cx,cy+height/2.0f},{cx-width/2.0f,cy-height/2.0f},{cx-width/4.0f,cy-height/4.0f},{cx+width/4.0f,cy-height/4.0f},{cx+width/2.0f,cy-height/2.0f}};
        SolidBrush br(Color(255,220,35,35)); g.FillPolygon(&br,pts,5);
        Pen pn(Color(255,140,20,20),1.5f*sc); g.DrawPolygon(&pn,pts,5);
        // 装甲板
        Pen ap(Color(255,180,50,50),1*sc); g.DrawLine(&ap,cx,cy-height/2.0f+4*sc,cx,cy+height/2.0f-4*sc);
        // 侧翼触角
        Pen wp(Color(255,200,30,30),2*sc);
        g.DrawLine(&wp,cx-width/2.0f,cy-height/4.0f,cx-width/2.0f-8*sc,cy+4*sc);
        g.DrawLine(&wp,cx+width/2.0f,cy-height/4.0f,cx+width/2.0f+8*sc,cy+4*sc);
        // 引擎点
        SolidBrush ep(Color(255,255,200,100)); g.FillEllipse(&ep,cx-width/2.0f-9*sc,cy+3*sc,4*sc,4*sc); g.FillEllipse(&ep,cx+width/2.0f+5*sc,cy+3*sc,4*sc,4*sc);
        // 眼睛
        SolidBrush eb(Color(255,255,200,200)); g.FillEllipse(&eb,cx-3*sc,cy-2*sc,6*sc,5*sc);
        SolidBrush ep2(Color(255,255,80,80)); g.FillEllipse(&ep2,cx-1*sc,cy-1*sc,2*sc,2*sc);
        break;
    }
    case EnemyType::FAST: {
        // 高速截击机 — 锐角三角 + 速度纹
        PointF pts[3]={{cx,cy+height/2.0f},{cx-width/2.0f,cy-height/2.0f},{cx+width/2.0f,cy-height/2.0f}};
        SolidBrush br(Color(255,255,160,0)); g.FillPolygon(&br,pts,3);
        Pen pn(Color(255,200,110,0),1.5f*sc); g.DrawPolygon(&pn,pts,3);
        // 速度线
        Pen sp(Color(255,255,200,60),1*sc);
        g.DrawLine(&sp,cx,cy-height/2.0f,cx,cy-height/2.0f-5*sc);
        g.DrawLine(&sp,cx-5*sc,cy-height/2.0f+3*sc,cx-6*sc,cy-height/2.0f-2*sc);
        g.DrawLine(&sp,cx+5*sc,cy-height/2.0f+3*sc,cx+6*sc,cy-height/2.0f-2*sc);
        // 引擎尾迹
        Pen tp(Color(255,255,180,40),1.5f*sc);
        g.DrawLine(&tp,cx,cy+height/2.0f-2*sc,cx,cy+height/2.0f+3*sc);
        break;
    }
    case EnemyType::SHOOTING: {
        // 重装炮艇 — 六边形 + 炮塔
        // 光晕
        for (int i=1;i>=0;i--) { Pen gp(Color(255,60-i*20,15,90),1+i); g.DrawEllipse(&gp,cx-24*sc,cy-24*sc,48*sc,48*sc); }
        // 机身六边形
        PointF hx[6]={{cx-12*sc,cy-15*sc},{cx+12*sc,cy-15*sc},{cx+17*sc,cy},{cx+12*sc,cy+15*sc},{cx-12*sc,cy+15*sc},{cx-17*sc,cy}};
        SolidBrush br(Color(255,155,35,215)); g.FillPolygon(&br,hx,6);
        Pen pn(Color(255,210,100,255),2*sc); g.DrawPolygon(&pn,hx,6);
        // 炮管
        Pen gn(Color(255,255,70,255),2.5f*sc);
        g.DrawLine(&gn,cx-6*sc,cy+15*sc,cx-6*sc,cy+15*sc+8*sc);
        g.DrawLine(&gn,cx+6*sc,cy+15*sc,cx+6*sc,cy+15*sc+8*sc);
        // 炮口闪光
        if (animFrame%20<5) { SolidBrush gf(Color(255,255,200,255)); g.FillEllipse(&gf,cx-7*sc,cy+15*sc+7*sc,3*sc,3*sc); g.FillEllipse(&gf,cx+5*sc,cy+15*sc+7*sc,3*sc,3*sc); }
        // 护甲环
        Pen ar(Color(255,200,80,240),1*sc); g.DrawEllipse(&ar,cx-10*sc,cy-8*sc,20*sc,16*sc);
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
