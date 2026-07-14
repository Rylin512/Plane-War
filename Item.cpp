#include "Item.h"

// ============================================================
// 文件：Item.cpp
// 功能：道具类实现
// ============================================================

Item::Item(float x, float y, ItemType _type)
    : GameObject(x, y, ITEM_WIDTH, ITEM_HEIGHT)
    , itemType(_type)
    , fallSpeed(ITEM_FALL_SPEED)
{
}

void Item::update() {
    y += fallSpeed;
}

void Item::render(HDC hdc) const {
    float cx = x + width / 2.0f;
    float cy = y + height / 2.0f;

    HBRUSH brush;
    HPEN pen;
    const char* label = "?";

    switch (itemType) {
    case ItemType::FIREPOWER:
        // 火力增强 — 黄色星形
        brush = CreateSolidBrush(RGB(255, 200, 0));
        pen = CreatePen(PS_SOLID, 1, RGB(255, 220, 50));
        label = "F";
        break;
    case ItemType::HEALTH:
        // 生命恢复 — 红色心形
        brush = CreateSolidBrush(RGB(255, 50, 50));
        pen = CreatePen(PS_SOLID, 1, RGB(255, 100, 100));
        label = "H";
        break;
    case ItemType::BOMB:
        // 炸弹 — 橙色
        brush = CreateSolidBrush(RGB(255, 120, 0));
        pen = CreatePen(PS_SOLID, 1, RGB(255, 160, 50));
        label = "B";
        break;
    case ItemType::SHIELD:
        // 护盾 — 蓝色菱形
        brush = CreateSolidBrush(RGB(0, 150, 255));
        pen = CreatePen(PS_SOLID, 1, RGB(50, 200, 255));
        label = "S";
        break;
    default:
        brush = CreateSolidBrush(RGB(150, 150, 150));
        pen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
        break;
    }

    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);

    // 绘制圆角矩形背景
    RoundRect(hdc, (int)x, (int)y, (int)(x + width), (int)(y + height), 6, 6);

    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);

    // 绘制文字标识
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));
    RECT rc = { (int)x, (int)y, (int)(x + width), (int)(y + height) };
    DrawTextA(hdc, label, 1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}
