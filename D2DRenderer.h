#pragma once
#include "Renderer.h"
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>
#include <map>
#include <string>

using Microsoft::WRL::ComPtr;

// ============================================================
// D2DRenderer.h — Direct2D 硬件加速渲染器
// 使用 ID2D1HwndRenderTarget (兼容性最好)
// ============================================================

class Direct2DRenderer : public Renderer {
public:
    Direct2DRenderer();
    ~Direct2DRenderer() override;

    bool init(HWND hwnd, int width, int height) override;
    void resize(int width, int height) override;
    void shutdown() override;

    void beginFrame() override;
    void clear(Gdiplus::Color c) override;
    void endFrame() override;

    void setSmoothing(bool on) override;
    void setPerfMode(bool on) override;

    void drawLine(float x1, float y1, float x2, float y2,
                  Gdiplus::Color c, float width) override;
    void drawEllipse(float x, float y, float w, float h,
                     Gdiplus::Color c, float lineWidth) override;
    void fillEllipse(float x, float y, float w, float h,
                     Gdiplus::Color c) override;
    void drawPolygon(const Gdiplus::PointF* pts, int n,
                     Gdiplus::Color c, float lineWidth) override;
    void fillPolygon(const Gdiplus::PointF* pts, int n,
                     Gdiplus::Color c) override;
    void drawRect(float x, float y, float w, float h,
                  Gdiplus::Color c, float lineWidth) override;
    void fillRect(float x, float y, float w, float h,
                  Gdiplus::Color c) override;

    void drawBitmap(Gdiplus::Image* img, float x, float y,
                    float w, float h) override;

    void drawText(const wchar_t* text, float x, float y,
                  const wchar_t* fontFamily, float fontSize,
                  int fontStyle, Gdiplus::Color c) override;
    void drawTextCentered(const wchar_t* text, float x, float y, float rw, float rh,
                          const wchar_t* fontFamily, float fontSize,
                          int fontStyle, Gdiplus::Color c) override;
    void drawTextLeft(const wchar_t* text, float x, float y, float rw, float rh,
                      const wchar_t* fontFamily, float fontSize,
                      int fontStyle, Gdiplus::Color c) override;

    void* getNative() override;
    int   getWidth() const override { return width; }
    int   getHeight() const override { return height; }

    bool isValid() const { return valid; }

private:
    bool valid;
    int  width, height;
    HWND hwnd;

    ComPtr<ID2D1Factory>       d2dFactory;
    ComPtr<ID2D1HwndRenderTarget> renderTarget;
    ComPtr<IDWriteFactory>     dwriteFactory;

    std::map<DWORD, ComPtr<ID2D1SolidColorBrush>> brushCache;
    std::map<std::wstring, ComPtr<IDWriteTextFormat>> textFormatCache;
    std::map<Gdiplus::Image*, ComPtr<ID2D1Bitmap>> bitmapCache;

    ID2D1SolidColorBrush* getBrush(Gdiplus::Color c);
    IDWriteTextFormat* getTextFormat(const wchar_t* family, float size, int style,
                                      int halign = 0, int valign = 0);
    ID2D1Bitmap* getBitmap(Gdiplus::Image* img);

    static D2D1_COLOR_F toD2DColor(Gdiplus::Color c);
    static D2D1_POINT_2F toD2DPoint(float x, float y);
    static D2D1_RECT_F toD2DRect(float x, float y, float w, float h);
    static D2D1_ELLIPSE toD2DEllipse(float x, float y, float w, float h);
    static DWRITE_FONT_WEIGHT toDWriteWeight(int gdiStyle);
};
