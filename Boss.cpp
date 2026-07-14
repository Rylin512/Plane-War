#include "Boss.h"
#include "Game.h"
#include "Player.h"
#include "Bullet.h"
#include <cmath>

using namespace Gdiplus;

Boss::Boss(float startX, int level)
    : Enemy(startX, -(float)BOSS_HEIGHT, BOSS_WIDTH, BOSS_HEIGHT, EnemyType::BOSS)
    , phase(BossPhase::ENTRY), phaseTimer(0), patternTimer(0), patternIndex(0)
    , targetX(WINDOW_WIDTH/2.0f), entryTimer(60), entered(false), animTimer(0)
{
    speed = BOSS_SPEED;
    hp = BOSS_BASE_HP + level*10; maxHp = hp;
    scoreValue = BOSS_SCORE + level*1000;
    type = EnemyType::BOSS;
}

BossPhase Boss::getCurrentPhase() const { return phase; }

void Boss::updatePhase() {
    float hpPct = (float)hp/maxHp;
    if (hpPct > 0.66f) phase = BossPhase::PHASE_1;
    else if (hpPct > 0.33f) { if (phase!=BossPhase::PHASE_2) { phase=BossPhase::PHASE_2; phaseTimer=0; patternIndex=0; } }
    else { if (phase!=BossPhase::PHASE_3) { phase=BossPhase::PHASE_3; phaseTimer=0; patternIndex=0; } }
}

void Boss::update() {
    animTimer++;
    if (!entered) {
        entryTimer--; y += 1.5f;
        if (y >= 50.0f) { y = 50.0f; entered = true; phase = BossPhase::PHASE_1; }
        return;
    }
    updatePhase();
    switch (phase) {
    case BossPhase::PHASE_1: targetX=WINDOW_WIDTH/2.0f+sinf(animTimer*0.02f)*150; x+=(targetX-x)*0.02f; break;
    case BossPhase::PHASE_2: targetX=WINDOW_WIDTH/2.0f+sinf(animTimer*0.04f)*200; x+=(targetX-x)*0.05f; break;
    case BossPhase::PHASE_3: targetX=WINDOW_WIDTH/2.0f+sinf(animTimer*0.06f)*180; x+=(targetX-x)*0.08f; break;
    default: break;
    }
    x = clamp(x, 0.0f, (float)(WINDOW_WIDTH-width));
    if (patternTimer>0) patternTimer--; phaseTimer++;
}

void Boss::firePattern(Game* game) {
    if (!entered||phase==BossPhase::DEFEATED||!game->player||patternTimer>0) return;
    float cx=centerX(), cy=y+height;
    switch (phase) {
    case BossPhase::PHASE_1:
        patternTimer=60; {
            float px=game->player->centerX(), py=game->player->centerY();
            float dx=px-cx, dy=py-cy, len=sqrtf(dx*dx+dy*dy);
            if (len>0) {
                float ba=dx/len;
                game->enemyBullets.push_back(new EnemyBullet(cx-BULLET_ENEMY_WIDTH/2.0f,cy,ba*0.5f));
                game->enemyBullets.push_back(new EnemyBullet(cx-BULLET_ENEMY_WIDTH/2.0f-8,cy,(ba-0.15f)*0.5f));
                game->enemyBullets.push_back(new EnemyBullet(cx-BULLET_ENEMY_WIDTH/2.0f+8,cy,(ba+0.15f)*0.5f));
            }
        } break;
    case BossPhase::PHASE_2:
        patternTimer=45;
        for (int i=-2;i<=2;i++) game->enemyBullets.push_back(new EnemyBullet(cx-BULLET_ENEMY_WIDTH/2.0f+i*10.0f,cy,i*0.2f*0.5f));
        if (rand()%3==0) { float dx=game->player->centerX()-cx; game->enemyBullets.push_back(new EnemyBullet(cx-BULLET_ENEMY_WIDTH/2.0f,cy,(dx/sqrtf(dx*dx+1))*0.5f)); }
        break;
    case BossPhase::PHASE_3:
        patternTimer=35; patternIndex=(patternIndex+1)%3;
        switch (patternIndex) {
        case 0: for (int i=0;i<8;i++) { float a=i*3.14159f*2/8; game->enemyBullets.push_back(new EnemyBullet(cx-BULLET_ENEMY_WIDTH/2.0f,cy,cosf(a)*0.6f)); } break;
        case 1: for (int i=-4;i<=4;i++) game->enemyBullets.push_back(new EnemyBullet(cx-BULLET_ENEMY_WIDTH/2.0f+i*8.0f,cy,i*0.15f*0.5f)); break;
        case 2: for (int i=0;i<6;i++) { float o=sinf((animTimer+i*10)*0.15f)*0.5f; game->enemyBullets.push_back(new EnemyBullet(cx-BULLET_ENEMY_WIDTH/2.0f-15+i*5.0f,cy,o)); } break;
        } break;
    default: break;
    }
}

void Boss::render(Graphics& g) const {
    float cx=centerX(), cy=centerY();
    Color bodyC, darkC;
    switch (phase) {
    case BossPhase::PHASE_1: bodyC=Palette::BossPhase1; darkC=Palette::BossPhase1Dark; break;
    case BossPhase::PHASE_2: bodyC=Palette::BossPhase2; darkC=Palette::BossPhase2Dark; break;
    case BossPhase::PHASE_3: bodyC=Palette::BossPhase3; darkC=Palette::BossPhase3Dark; break;
    default: bodyC=Palette::BossPhase1; darkC=Palette::BossPhase1Dark; break;
    }

    // Phase 3 愤怒光晕
    if (phase==BossPhase::PHASE_3) {
        int pulse=(int)(sinf(animTimer*0.2f)*10);
        for (int i=2;i>=0;i--) {
            float r=48+pulse+i*5;
            Pen pn(Color(100-i*30,255,50,10),2.0f+i);
            g.DrawEllipse(&pn,cx-r,cy-r,r*2,r*2);
        }
    }

    // 引擎火焰
    if (entered) {
        for (int i=0;i<3;i++) {
            int fl=6+rand()%10;
            Pen fo(Color(255,255,120+rand()%80,0),3);
            g.DrawLine(&fo,cx-18+i*18.0f,y+height,cx-18+i*18.0f,y+height+fl);
            Pen fi(Color(255,255,255,50),1);
            g.DrawLine(&fi,cx-18+i*18.0f,y+height,cx-18+i*18.0f,y+height+fl/2);
        }
    }

    // 阴影 / 主体
    PointF body[8]={{cx,y},{cx-30,y+15},{cx-35,y+25},{cx-25,y+55},{cx+25,y+55},{cx+35,y+25},{cx+30,y+15}};
    SolidBrush shBr(darkC); g.FillPolygon(&shBr,body,8);
    PointF body2[8]={{cx,y},{cx-30,y+15},{cx-35,y+25},{cx-25,y+55},{cx+25,y+55},{cx+35,y+25},{cx+30,y+15}};
    SolidBrush bdBr(bodyC); g.FillPolygon(&bdBr,body2,8);
    Pen bdPn(darkC,2); g.DrawPolygon(&bdPn,body2,8);

    // 机翼
    SolidBrush wBr(darkC);
    PointF lw[5]={{cx-30,cy-5},{cx-70,cy},{cx-65,cy+20},{cx-35,cy+15},{cx-30,cy-5}};
    PointF rw[5]={{cx+30,cy-5},{cx+70,cy},{cx+65,cy+20},{cx+35,cy+15},{cx+30,cy-5}};
    g.FillPolygon(&wBr,lw,5); g.FillPolygon(&wBr,rw,5);

    // 驾驶舱
    SolidBrush cg(Color(255,0,80,150)); g.FillEllipse(&cg,cx-13.0f,y+3.0f,26.0f,21.0f);
    SolidBrush cp(Palette::BossCockpit); g.FillEllipse(&cp,cx-10.0f,y+5.0f,20.0f,17.0f);

    // 护甲闪烁
    if ((phase==BossPhase::PHASE_2||phase==BossPhase::PHASE_3)&&animTimer%30<15) {
        Pen ap(Color(255,255,255,120),2); g.DrawLine(&ap,cx-25,y+20,cx+25,y+20);
    }
}

void Boss::renderHPBar(Graphics& g) const {
    if (!entered) return;
    float bw=220,bh=14,bx=WINDOW_WIDTH/2.0f-bw/2,by=4;
    float hpPct=(float)hp/maxHp; if(hpPct<0)hpPct=0;

    SolidBrush bgBr(Color(255,40,40,40)); g.FillRectangle(&bgBr,bx,by,bw,bh);
    Color hpC = hpPct>0.5f ? Color(255,(int)(255*(1-hpPct)*2),220,0) : Color(255,255,(int)(220*hpPct*2),0);
    SolidBrush hpBr(hpC); g.FillRectangle(&hpBr,bx,by,bw*hpPct,bh);
    SolidBrush hl(Color(80,255,255,255)); g.FillRectangle(&hl,bx,by,bw*hpPct,3.0f);
    Pen bdPn(Color(255,130,130,130),2); g.DrawRectangle(&bdPn,bx,by,bw,bh);

    FontFamily ff(L"Arial"); Font fnt(&ff,11,FontStyleBold,UnitPixel);
    SolidBrush wh(Color::White); StringFormat sf; sf.SetAlignment(StringAlignmentCenter);
    wchar_t buf[64]; swprintf_s(buf, _countof(buf), L"BOSS  %d / %d",hp,maxHp);
    g.DrawString(buf,-1,&fnt,RectF(bx,by,bw,bh),&sf,&wh);

    if (phase==BossPhase::PHASE_2||phase==BossPhase::PHASE_3) {
        SolidBrush ph(Color(255,phase==BossPhase::PHASE_3?255:255,phase==BossPhase::PHASE_3?40:180,0));
        const wchar_t* pt=phase==BossPhase::PHASE_3?L"CRITICAL":L"DANGER";
        g.DrawString(pt,-1,&fnt,RectF(bx,by+bh+3,bw,by+bh+16),&sf,&ph);
    }
}
