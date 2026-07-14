#include "Boss.h"
#include "Game.h"
#include "Player.h"
#include "Bullet.h"
#include <cmath>

using namespace Gdiplus;

Boss::Boss(float startX, int level)
    : Enemy(startX, -(float)BOSS_HEIGHT * GAME_SCALE,
            (int)(BOSS_WIDTH * GAME_SCALE), (int)(BOSS_HEIGHT * GAME_SCALE), EnemyType::BOSS)
    , phase(BossPhase::ENTRY), phaseTimer(0), patternTimer(0), patternIndex(0)
    , targetX(WINDOW_WIDTH/2.0f), entryTimer(60), entered(false), animTimer(0)
{
    speed = BOSS_SPEED * GAME_SCALE;
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
                game->enemyBullets.push_back(game->acquireEnemyBullet(cx-BULLET_ENEMY_WIDTH/2.0f,cy,ba*0.5f));
                game->enemyBullets.push_back(game->acquireEnemyBullet(cx-BULLET_ENEMY_WIDTH/2.0f-8,cy,(ba-0.15f)*0.5f));
                game->enemyBullets.push_back(game->acquireEnemyBullet(cx-BULLET_ENEMY_WIDTH/2.0f+8,cy,(ba+0.15f)*0.5f));
            }
        } break;
    case BossPhase::PHASE_2:
        patternTimer=45;
        for (int i=-2;i<=2;i++) game->enemyBullets.push_back(game->acquireEnemyBullet(cx-BULLET_ENEMY_WIDTH/2.0f+i*10.0f,cy,i*0.2f*0.5f));
        if (rand()%3==0) { float dx=game->player->centerX()-cx; game->enemyBullets.push_back(game->acquireEnemyBullet(cx-BULLET_ENEMY_WIDTH/2.0f,cy,(dx/sqrtf(dx*dx+1))*0.5f)); }
        break;
    case BossPhase::PHASE_3:
        patternTimer=35; patternIndex=(patternIndex+1)%3;
        switch (patternIndex) {
        case 0: for (int i=0;i<8;i++) { float a=i*3.14159f*2/8; game->enemyBullets.push_back(game->acquireEnemyBullet(cx-BULLET_ENEMY_WIDTH/2.0f,cy,cosf(a)*0.6f)); } break;
        case 1: for (int i=-4;i<=4;i++) game->enemyBullets.push_back(game->acquireEnemyBullet(cx-BULLET_ENEMY_WIDTH/2.0f+i*8.0f,cy,i*0.15f*0.5f)); break;
        case 2: for (int i=0;i<6;i++) { float o=sinf((animTimer+i*10)*0.15f)*0.5f; game->enemyBullets.push_back(game->acquireEnemyBullet(cx-BULLET_ENEMY_WIDTH/2.0f-15+i*5.0f,cy,o)); } break;
        } break;
    default: break;
    }
}

void Boss::render(Renderer& r) const {
    float s = GAME_SCALE;
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
        int pulse=(int)(sinf(animTimer*0.2f)*10*s);
        for (int i=2;i>=0;i--) {
            float radius=(48+pulse+i*5)*s;
            r.drawEllipse(cx-radius,cy-radius,radius*2,radius*2, Color(100-i*30,255,50,10), (2.0f+i)*s);
        }
    }

    // 引擎火焰
    if (entered) {
        for (int i=0;i<3;i++) {
            int fl=(int)((6+rand()%10)*s);
            r.drawLine(cx-(18-i*18)*s, y+height, cx-(18-i*18)*s, y+height+fl, Color(255,255,(BYTE)(120+rand()%80),0), 3*s);
            r.drawLine(cx-(18-i*18)*s, y+height, cx-(18-i*18)*s, y+height+fl/2, Color(255,255,255,50), 1*s);
        }
    }

    // 阴影 / 主体
    PointF body[8]={{cx,y},{cx-30*s,y+15*s},{cx-35*s,y+25*s},{cx-25*s,y+55*s},{cx+25*s,y+55*s},{cx+35*s,y+25*s},{cx+30*s,y+15*s}};
    r.fillPolygon(body,8,darkC);
    r.fillPolygon(body,8,bodyC);
    r.drawPolygon(body,8,darkC,2*s);

    // 机翼
    PointF lw[5]={{cx-30*s,cy-5*s},{cx-70*s,cy},{cx-65*s,cy+20*s},{cx-35*s,cy+15*s},{cx-30*s,cy-5*s}};
    PointF rw[5]={{cx+30*s,cy-5*s},{cx+70*s,cy},{cx+65*s,cy+20*s},{cx+35*s,cy+15*s},{cx+30*s,cy-5*s}};
    r.fillPolygon(lw,5,darkC); r.fillPolygon(rw,5,darkC);

    // 驾驶舱
    r.fillEllipse(cx-13*s,y+3*s,26*s,21*s, Color(255,0,80,150));
    r.fillEllipse(cx-10*s,y+5*s,20*s,17*s, Palette::BossCockpit);

    // 护甲闪烁
    if ((phase==BossPhase::PHASE_2||phase==BossPhase::PHASE_3)&&animTimer%30<15) {
        r.drawLine(cx-25*s,y+20*s,cx+25*s,y+20*s, Color(255,255,255,120), 2*s);
    }
}

void Boss::renderHPBar(Renderer& r) const {
    if (!entered) return;
    float uiSc = (std::max)(0.8f, (std::min)(WINDOW_WIDTH / 640.0f, 2.2f));
    float bw=220*uiSc, bh=14*uiSc, bx=WINDOW_WIDTH/2.0f-bw/2, by=8*uiSc;
    float hpPct=(float)hp/maxHp; if(hpPct<0)hpPct=0;

    r.fillRect(bx,by,bw,bh, Color(255,40,40,40));
    Color hpC = hpPct>0.5f ? Color(255,(int)(255*(1-hpPct)*2),220,0) : Color(255,255,(int)(220*hpPct*2),0);
    r.fillRect(bx,by,bw*hpPct,bh, hpC);
    r.fillRect(bx,by,bw*hpPct,3.0f*uiSc, Color(80,255,255,255));
    r.drawRect(bx,by,bw,bh, Color(255,130,130,130),2*uiSc);

    int fs = (int)(11 * uiSc);
    wchar_t buf[64]; swprintf_s(buf, _countof(buf), L"BOSS  %d / %d",hp,maxHp);
    r.drawTextCentered(buf, bx, by, bw, bh, L"Arial", (float)fs, FontStyleBold, Color::White);

    if (phase==BossPhase::PHASE_2||phase==BossPhase::PHASE_3) {
        Color ph(255,phase==BossPhase::PHASE_3?255:255,phase==BossPhase::PHASE_3?40:180,0);
        const wchar_t* pt=phase==BossPhase::PHASE_3?L"CRITICAL":L"DANGER";
        r.drawTextCentered(pt, bx, by+bh+3*uiSc, bw, 16*uiSc, L"Arial", (float)fs, FontStyleBold, ph);
    }
}
