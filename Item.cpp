#include "Item.h"
using namespace Gdiplus;

Item::Item(float x, float y, ItemType _type)
    : GameObject(x,y,ITEM_SIZE,ITEM_SIZE), itemType(_type), fallSpeed(ITEM_FALL_SPEED*GAME_SCALE) {}

void Item::update() { y += fallSpeed; }

void Item::render(Graphics& g) const {
    float sc = GAME_SCALE, cx=centerX(), cy=centerY(), sz = ITEM_SIZE*sc;
    Color fill, glow, border; const wchar_t* lbl=L"?";
    switch (itemType) {
    case ItemType::FIREPOWER: fill=Palette::ItemFirepower; glow=Color(255,255,240,100); border=Color(255,255,180,20); lbl=L"F"; break;
    case ItemType::HEALTH:    fill=Palette::ItemHealth;    glow=Color(255,255,140,140); border=Color(255,220,30,30);  lbl=L"H"; break;
    case ItemType::BOMB:      fill=Palette::ItemBomb;      glow=Color(255,255,200,100); border=Color(255,230,90,0);   lbl=L"B"; break;
    case ItemType::SHIELD:    fill=Palette::ItemShield;    glow=Color(255,100,220,255); border=Color(255,0,120,220);  lbl=L"S"; break;
    case ItemType::FIRERATE:  fill=Color(255,0,255,100);  glow=Color(255,100,255,150); border=Color(255,0,200,80);   lbl=L"R"; break;
    default: fill=Color(255,150,150,150); glow=Color(255,200,200,200); border=Color(255,100,100,100); break;
    }
    // 光晕
    Pen gp(glow,2*sc); g.DrawEllipse(&gp,cx-13*sc,cy-13*sc,26*sc,26*sc);
    // 主体
    SolidBrush fb(fill); g.FillRectangle(&fb,x,y,sz,sz);
    Pen bp(border,2*sc); g.DrawRectangle(&bp,x,y,sz,sz);
    // 高光
    Pen hp(Color(255,255,255,255),1*sc); g.DrawLine(&hp,x+3*sc,y+3*sc,x+sz-3*sc,y+3*sc);
    // 标签
    FontFamily ff(L"Arial"); Font fnt(&ff,12*sc,FontStyleBold,UnitPixel);
    SolidBrush wh(Color::White); StringFormat sf; sf.SetAlignment(StringAlignmentCenter); sf.SetLineAlignment(StringAlignmentCenter);
    g.DrawString(lbl,-1,&fnt,RectF(x,y,sz,sz),&sf,&wh);
}
