#pragma once
#include <windows.h>
#include <gdiplus.h>
#include "Config.h"
#include "GameObject.h"

enum class ItemType { FIREPOWER, HEALTH, BOMB, SHIELD, FIRERATE };

// ============================================================
// 文件：Item.h
// 功能：道具 — GDI+ 发光渲染
// ============================================================

class Item : public GameObject {
public:
    Item(float x, float y, ItemType type);
    virtual ~Item() {}
    virtual void update() override;
    virtual void render(Gdiplus::Graphics& g) const override;

    ItemType itemType;
    float fallSpeed;
};
