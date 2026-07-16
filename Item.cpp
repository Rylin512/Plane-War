#include "Item.h"
#include "Renderer.h"
using namespace Gdiplus;

Item::Item(float x, float y, ItemType _type)
    : GameObject(x, y, (int)(ITEM_SIZE*GAME_SCALE), (int)(ITEM_SIZE*GAME_SCALE)),
      itemType(_type), fallSpeed(ITEM_FALL_SPEED*GAME_SCALE) {}

void Item::update() { y += fallSpeed; }

void Item::render(Renderer& r) const {
    float sc = GAME_SCALE, cx = centerX(), cy = centerY();
    float rOuter = (ITEM_SIZE/2.0f + 4) * sc;   // 光环外径
    float rInner = (ITEM_SIZE/2.0f) * sc;        // 主体半边长
    Color fill, glow, border; const wchar_t* lbl = L"?";
    switch (itemType) {
    case ItemType::FIREPOWER: fill=Palette::ItemFirepower; glow=Color(255,255,240,100); border=Color(255,255,180,20); lbl=L"F"; break;
    case ItemType::HEALTH:    fill=Palette::ItemHealth;    glow=Color(255,255,140,140); border=Color(255,220,30,30);  lbl=L"H"; break;
    case ItemType::BOMB:      fill=Palette::ItemBomb;      glow=Color(255,255,200,100); border=Color(255,230,90,0);   lbl=L"B"; break;
    case ItemType::SHIELD:    fill=Palette::ItemShield;    glow=Color(255,100,220,255); border=Color(255,0,120,220);  lbl=L"S"; break;
    case ItemType::FIRERATE:  fill=Color(255,0,255,100);  glow=Color(255,100,255,150); border=Color(255,0,200,80);   lbl=L"R"; break;
    default: fill=Color(255,150,150,150); glow=Color(255,200,200,200); border=Color(255,100,100,100); break;
    }
    // 光晕圆框（光环始终以 centerX/centerY 为圆心）
    r.drawEllipse(cx - rOuter, cy - rOuter, rOuter * 2, rOuter * 2, glow, 2 * sc);
    // 主体矩形（以 x/y 为左上角，宽高已缩放）
    float sz = (float)width;
    r.fillRect(x, y, sz, sz, fill);
    r.drawRect(x, y, sz, sz, border, 2 * sc);
    // 高光
    r.drawLine(x + 3 * sc, y + 3 * sc, x + sz - 3 * sc, y + 3 * sc, Color(255, 255, 255, 255), 1 * sc);
    // 标签（使用 centerX/centerY 精确居中）
    r.drawTextCentered(lbl, cx - rInner, cy - rInner, rInner * 2, rInner * 2, L"Arial", 12 * sc, FontStyleBold, Color::White);
}
