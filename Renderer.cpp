#include "Renderer.h"
#include <map>
using namespace Gdiplus;

// ============================================================
// GdiplusRenderer — 封装现有 GDI+ 调用到 Renderer 接口
// 内置画刷/画笔缓存，避免每帧分配
// ============================================================

class GdiplusRenderer : public Renderer {
public:
    Graphics* g;
    int w, h;

private:
    // 画刷缓存 (ARGB -> SolidBrush*)
    std::map<DWORD, SolidBrush*> brushCache;
    // 画笔缓存 (ARGB + width key -> Pen*)
    std::map<DWORD, Pen*> penCache;

    SolidBrush* getBrush(Color c) {
        DWORD k = c.GetValue();
        auto it = brushCache.find(k);
        if (it != brushCache.end()) return it->second;
        SolidBrush* b = new SolidBrush(c);
        brushCache[k] = b;
        return b;
    }

    Pen* getPen(Color c, float width) {
        DWORD k = c.GetValue() ^ ((DWORD)(width * 100) << 16);
        auto it = penCache.find(k);
        if (it != penCache.end()) return it->second;
        Pen* p = new Pen(c, width);
        penCache[k] = p;
        return p;
    }

    void clearCaches() {
        for (auto& kv : brushCache) delete kv.second;
        for (auto& kv : penCache) delete kv.second;
        brushCache.clear();
        penCache.clear();
    }

public:
    GdiplusRenderer() : g(nullptr), w(0), h(0) {}
    ~GdiplusRenderer() { clearCaches(); }

    bool init(HWND, int width, int height) override { w = width; h = height; return g != nullptr; }
    void resize(int width, int height) override { w = width; h = height; }
    void shutdown() override { clearCaches(); g = nullptr; }

    void beginFrame() override {}
    void clear(Color c) override { if (g) g->Clear(c); }
    void endFrame() override { if (g) g->Flush(); }

    void setSmoothing(bool on) override {
        if (!g) return;
        g->SetSmoothingMode(on ? SmoothingModeAntiAlias : SmoothingModeHighSpeed);
        g->SetTextRenderingHint(on ? TextRenderingHintAntiAlias : TextRenderingHintAntiAliasGridFit);
    }
    void setPerfMode(bool on) override {
        if (!g) return;
        g->SetSmoothingMode(on ? SmoothingModeHighSpeed : SmoothingModeAntiAlias);
        g->SetTextRenderingHint(on ? TextRenderingHintSingleBitPerPixelGridFit : TextRenderingHintAntiAlias);
    }

    void drawLine(float x1, float y1, float x2, float y2, Color c, float width) override {
        if (!g) return;
        g->DrawLine(getPen(c, width), x1, y1, x2, y2);
    }
    void drawEllipse(float x, float y, float w, float h, Color c, float lw) override {
        if (!g) return;
        g->DrawEllipse(getPen(c, lw), x, y, w, h);
    }
    void fillEllipse(float x, float y, float w, float h, Color c) override {
        if (!g) return;
        g->FillEllipse(getBrush(c), x, y, w, h);
    }
    void drawPolygon(const PointF* pts, int n, Color c, float lw) override {
        if (!g) return;
        g->DrawPolygon(getPen(c, lw), pts, n);
    }
    void fillPolygon(const PointF* pts, int n, Color c) override {
        if (!g) return;
        g->FillPolygon(getBrush(c), pts, n);
    }
    void drawRect(float x, float y, float w, float h, Color c, float lw) override {
        if (!g) return;
        g->DrawRectangle(getPen(c, lw), x, y, w, h);
    }
    void fillRect(float x, float y, float w, float h, Color c) override {
        if (!g) return;
        g->FillRectangle(getBrush(c), x, y, w, h);
    }

    void drawBitmap(Image* img, float x, float y, float w, float h) override {
        if (!g || !img) return;
        g->DrawImage(img, x, y, w, h);
    }

    void drawText(const wchar_t* text, float x, float y,
                  const wchar_t* family, float size, int style, Color c) override {
        if (!g) return;
        FontFamily ff(family);
        Font f(&ff, size, style, UnitPixel);
        g->DrawString(text, -1, &f, PointF(x, y), getBrush(c));
    }

    void drawTextCentered(const wchar_t* text, float x, float y, float rw, float rh,
                          const wchar_t* family, float size, int style, Color c) override {
        if (!g) return;
        FontFamily ff(family);
        Font f(&ff, size, style, UnitPixel);
        StringFormat sf;
        sf.SetAlignment(StringAlignmentCenter);
        sf.SetLineAlignment(StringAlignmentCenter);
        RectF rc(x, y, rw, rh);
        g->DrawString(text, -1, &f, rc, &sf, getBrush(c));
    }

    void drawTextLeft(const wchar_t* text, float x, float y, float rw, float rh,
                      const wchar_t* family, float size, int style, Color c) override {
        if (!g) return;
        FontFamily ff(family);
        Font f(&ff, size, style, UnitPixel);
        StringFormat sf;
        sf.SetAlignment(StringAlignmentNear);
        sf.SetLineAlignment(StringAlignmentCenter);
        RectF rc(x, y, rw, rh);
        g->DrawString(text, -1, &f, rc, &sf, getBrush(c));
    }

    void* getNative() override { return g; }
    int   getWidth() const override { return w; }
    int   getHeight() const override { return h; }
};

// 工厂函数
Renderer* CreateGdiplusRenderer(Graphics* graphics, int width, int height) {
    GdiplusRenderer* r = new GdiplusRenderer();
    r->g = graphics;
    r->w = width;
    r->h = height;
    return r;
}
