#pragma once
#include <windows.h>
#include "Config.h"
#include "GameObject.h"

// ============================================================
// 文件：Item.h
// 功能：道具类 — 火力增强、生命恢复、炸弹、护盾
// ============================================================

enum class ItemType { FIREPOWER, HEALTH, BOMB, SHIELD };

class Item : public GameObject {
public:
    Item(float x, float y, ItemType type);
    virtual ~Item() {}

    virtual void update() override;
    virtual void render(HDC hdc) const;

    ItemType itemType;
    float fallSpeed;
};
