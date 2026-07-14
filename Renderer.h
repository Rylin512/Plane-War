#pragma once
#include <windows.h>
#include <gdiplus.h>

// ============================================================
// Renderer.h — 抽象渲染接口（GDI+ / Direct2D 双后端）
// ============================================================

class Renderer {
public:
    virtual ~Renderer() {}

    // ── 生命周期 ──
    virtual bool init(HWND hwnd, int width, int height) = 0;
    virtual void resize(int width, int height) = 0;
    virtual void shutdown() = 0;

    // ── 帧控制 ──
    virtual void beginFrame() = 0;
    virtual void clear(Gdiplus::Color c) = 0;
    virtual void endFrame() = 0;

    // ── 质量设置 ──
    virtual void setSmoothing(bool on) = 0;
    virtual void setPerfMode(bool on) = 0;

    // ── 基本图元 ──
    virtual void drawLine(float x1, float y1, float x2, float y2,
                          Gdiplus::Color c, float width) = 0;
    virtual void drawEllipse(float x, float y, float w, float h,
                             Gdiplus::Color c, float lineWidth) = 0;
    virtual void fillEllipse(float x, float y, float w, float h,
                             Gdiplus::Color c) = 0;
    virtual void drawPolygon(const Gdiplus::PointF* pts, int n,
                             Gdiplus::Color c, float lineWidth) = 0;
    virtual void fillPolygon(const Gdiplus::PointF* pts, int n,
                             Gdiplus::Color c) = 0;
    virtual void drawRect(float x, float y, float w, float h,
                          Gdiplus::Color c, float lineWidth) = 0;
    virtual void fillRect(float x, float y, float w, float h,
                          Gdiplus::Color c) = 0;

    // ── 图片 ──
    virtual void drawBitmap(Gdiplus::Image* img, float x, float y,
                            float w, float h) = 0;

    // ── 文字（点定位） ──
    virtual void drawText(const wchar_t* text, float x, float y,
                          const wchar_t* fontFamily, float fontSize,
                          int fontStyle, Gdiplus::Color c) = 0;

    // ── 文字（矩形内居中） ──
    virtual void drawTextCentered(const wchar_t* text,
                                  float x, float y, float w, float h,
                                  const wchar_t* fontFamily, float fontSize,
                                  int fontStyle, Gdiplus::Color c) = 0;

    // ── 文字（矩形内左对齐） ──
    virtual void drawTextLeft(const wchar_t* text,
                              float x, float y, float w, float h,
                              const wchar_t* fontFamily, float fontSize,
                              int fontStyle, Gdiplus::Color c) = 0;

    // ── 原生句柄 ──
    virtual void* getNative() = 0;
    virtual int   getWidth() const = 0;
    virtual int   getHeight() const = 0;
};
