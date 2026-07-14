#include "D2DRenderer.h"
using namespace Gdiplus;

// ============================================================
// Direct2DRenderer — Direct2D HwndRenderTarget 实现
// ============================================================

Direct2DRenderer::Direct2DRenderer() : valid(false), width(0), height(0), hwnd(nullptr) {}
Direct2DRenderer::~Direct2DRenderer() { shutdown(); }

// ── 颜色转换 ──

D2D1_COLOR_F Direct2DRenderer::toD2DColor(Color c) {
    return D2D1::ColorF(c.GetR() / 255.0f, c.GetG() / 255.0f,
                        c.GetB() / 255.0f, c.GetA() / 255.0f);
}

D2D1_POINT_2F Direct2DRenderer::toD2DPoint(float x, float y) { return D2D1::Point2F(x, y); }

D2D1_RECT_F Direct2DRenderer::toD2DRect(float x, float y, float w, float h) {
    return D2D1::RectF(x, y, x + w, y + h);
}

D2D1_ELLIPSE Direct2DRenderer::toD2DEllipse(float x, float y, float w, float h) {
    D2D1_ELLIPSE e;
    e.point = D2D1::Point2F(x + w / 2, y + h / 2);
    e.radiusX = w / 2; e.radiusY = h / 2;
    return e;
}

DWRITE_FONT_WEIGHT Direct2DRenderer::toDWriteWeight(int gdiStyle) {
    return (gdiStyle & FontStyleBold) ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_REGULAR;
}

// ── 画刷缓存 ──

ID2D1SolidColorBrush* Direct2DRenderer::getBrush(Color c) {
    DWORD argb = c.GetValue();
    auto it = brushCache.find(argb);
    if (it != brushCache.end()) return it->second.Get();
    ComPtr<ID2D1SolidColorBrush> brush;
    renderTarget->CreateSolidColorBrush(toD2DColor(c), &brush);
    brushCache[argb] = brush;
    return brush.Get();
}

// ── 文字格式缓存 ──

IDWriteTextFormat* Direct2DRenderer::getTextFormat(const wchar_t* family, float size, int style,
                                                     int halign, int valign) {
    wchar_t key[128];
    swprintf_s(key, _countof(key), L"%s_%.1f_%d_%d_%d", family, size, style, halign, valign);
    std::wstring ks(key);
    auto it = textFormatCache.find(ks);
    if (it != textFormatCache.end()) return it->second.Get();
    ComPtr<IDWriteTextFormat> fmt;
    dwriteFactory->CreateTextFormat(family, nullptr,
        toDWriteWeight(style),
        (style & FontStyleItalic) ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, size, L"", &fmt);
    // 立即设置对齐（不可变属性，创建后设置）
    switch (halign) {
    case 1: fmt->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER); break;
    case 2: fmt->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING); break;
    default: fmt->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING); break;
    }
    switch (valign) {
    case 1: fmt->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER); break;
    case 2: fmt->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR); break;
    default: fmt->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR); break;
    }
    textFormatCache[ks] = fmt;
    return fmt.Get();
}

// ── 位图缓存 ──

ID2D1Bitmap* Direct2DRenderer::getBitmap(Gdiplus::Image* img) {
    if (!img) return nullptr;
    auto it = bitmapCache.find(img);
    if (it != bitmapCache.end()) return it->second.Get();

    Bitmap* bmp = static_cast<Bitmap*>(img);
    if (!bmp) return nullptr;
    int iw = bmp->GetWidth(), ih = bmp->GetHeight();
    if (iw <= 0 || ih <= 0) return nullptr;

    BitmapData bmpData;
    Rect rc(0, 0, iw, ih);
    bmp->LockBits(&rc, ImageLockModeRead, PixelFormat32bppARGB, &bmpData);

    D2D1_BITMAP_PROPERTIES bp = D2D1::BitmapProperties(
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
    ComPtr<ID2D1Bitmap> d2dBmp;
    renderTarget->CreateBitmap(D2D1::SizeU(iw, ih), bmpData.Scan0, bmpData.Stride, bp, &d2dBmp);
    bmp->UnlockBits(&bmpData);
    bitmapCache[img] = d2dBmp;
    return d2dBmp.Get();
}

// ── 初始化 ──

bool Direct2DRenderer::init(HWND hw, int w, int h) {
    width = w; height = h; hwnd = hw;
    ID2D1Factory* factory = nullptr;
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &factory);
    d2dFactory = factory;
    if (FAILED(hr)) return false;

    RECT rc; GetClientRect(hwnd, &rc);
    D2D1_SIZE_U sz = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
    hr = d2dFactory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_HARDWARE,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)),
        D2D1::HwndRenderTargetProperties(hwnd, sz), &renderTarget);
    if (FAILED(hr)) {
        // 尝试软件渲染
        hr = d2dFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_SOFTWARE,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)),
            D2D1::HwndRenderTargetProperties(hwnd, sz), &renderTarget);
        if (FAILED(hr)) return false;
    }

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), &dwriteFactory);
    if (FAILED(hr)) return false;

    valid = true;
    return true;
}

void Direct2DRenderer::resize(int w, int h) {
    width = w; height = h;
    if (renderTarget) {
        D2D1_SIZE_U sz = D2D1::SizeU(width, height);
        renderTarget->Resize(sz);
    }
}

void Direct2DRenderer::shutdown() {
    bitmapCache.clear();
    brushCache.clear();
    textFormatCache.clear();
    renderTarget.Reset();
    d2dFactory.Reset();
    dwriteFactory.Reset();
    valid = false;
}

// ── 帧控制 ──

void Direct2DRenderer::beginFrame() {
    if (!renderTarget) return;
    renderTarget->BeginDraw();
}

void Direct2DRenderer::clear(Color c) {
    if (!renderTarget) return;
    renderTarget->Clear(toD2DColor(c));
}

void Direct2DRenderer::endFrame() {
    if (!renderTarget) return;
    renderTarget->EndDraw();
}

void* Direct2DRenderer::getNative() { return renderTarget.Get(); }

// ── 质量设置 ──

void Direct2DRenderer::setSmoothing(bool on) {
    if (!renderTarget) return;
    renderTarget->SetAntialiasMode(on ? D2D1_ANTIALIAS_MODE_PER_PRIMITIVE : D2D1_ANTIALIAS_MODE_ALIASED);
}

void Direct2DRenderer::setPerfMode(bool on) {
    if (!renderTarget) return;
    renderTarget->SetAntialiasMode(on ? D2D1_ANTIALIAS_MODE_ALIASED : D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
}

// ── 图元 ──

void Direct2DRenderer::drawLine(float x1, float y1, float x2, float y2, Color c, float w) {
    if (!renderTarget) return;
    renderTarget->DrawLine(toD2DPoint(x1, y1), toD2DPoint(x2, y2), getBrush(c), w);
}

void Direct2DRenderer::drawEllipse(float x, float y, float w, float h, Color c, float lw) {
    if (!renderTarget) return;
    renderTarget->DrawEllipse(toD2DEllipse(x, y, w, h), getBrush(c), lw);
}

void Direct2DRenderer::fillEllipse(float x, float y, float w, float h, Color c) {
    if (!renderTarget) return;
    renderTarget->FillEllipse(toD2DEllipse(x, y, w, h), getBrush(c));
}

void Direct2DRenderer::drawPolygon(const PointF* pts, int n, Color c, float lw) {
    if (!renderTarget || n < 2) return;
    ComPtr<ID2D1PathGeometry> geo;
    d2dFactory->CreatePathGeometry(geo.GetAddressOf());
    ComPtr<ID2D1GeometrySink> sink;
    geo->Open(sink.GetAddressOf());
    sink->BeginFigure(D2D1::Point2F(pts[0].X, pts[0].Y), D2D1_FIGURE_BEGIN_FILLED);
    for (int i = 1; i < n; i++)
        sink->AddLine(D2D1::Point2F(pts[i].X, pts[i].Y));
    sink->EndFigure(D2D1_FIGURE_END_CLOSED);
    sink->Close();
    renderTarget->DrawGeometry(geo.Get(), getBrush(c), lw);
}

void Direct2DRenderer::fillPolygon(const PointF* pts, int n, Color c) {
    if (!renderTarget || n < 2) return;
    ComPtr<ID2D1PathGeometry> geo;
    d2dFactory->CreatePathGeometry(geo.GetAddressOf());
    ComPtr<ID2D1GeometrySink> sink;
    geo->Open(sink.GetAddressOf());
    sink->BeginFigure(D2D1::Point2F(pts[0].X, pts[0].Y), D2D1_FIGURE_BEGIN_FILLED);
    for (int i = 1; i < n; i++)
        sink->AddLine(D2D1::Point2F(pts[i].X, pts[i].Y));
    sink->EndFigure(D2D1_FIGURE_END_CLOSED);
    sink->Close();
    renderTarget->FillGeometry(geo.Get(), getBrush(c));
}

void Direct2DRenderer::drawRect(float x, float y, float w, float h, Color c, float lw) {
    if (!renderTarget) return;
    renderTarget->DrawRectangle(toD2DRect(x, y, w, h), getBrush(c), lw);
}

void Direct2DRenderer::fillRect(float x, float y, float w, float h, Color c) {
    if (!renderTarget) return;
    renderTarget->FillRectangle(toD2DRect(x, y, w, h), getBrush(c));
}

// ── 图片 ──

void Direct2DRenderer::drawBitmap(Gdiplus::Image* img, float x, float y, float w, float h) {
    if (!renderTarget || !img) return;
    ID2D1Bitmap* bmp = getBitmap(img);
    if (!bmp) return;
    renderTarget->DrawBitmap(bmp, toD2DRect(x, y, w, h), 1.0f,
                             D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
}

// ── 文字 ──

void Direct2DRenderer::drawText(const wchar_t* text, float x, float y,
                                 const wchar_t* family, float size, int style, Color c) {
    if (!renderTarget) return;
    // 使用左对齐+顶部对齐的格式（不修改共享缓存）
    IDWriteTextFormat* fmt = getTextFormat(family, size, style, 0, 0);
    D2D1_RECT_F rc = D2D1::RectF(x, y, x + (float)width - x, y + size * 2);
    renderTarget->DrawText(text, (UINT32)wcslen(text), fmt, rc, getBrush(c));
}

void Direct2DRenderer::drawTextCentered(const wchar_t* text, float x, float y, float rw, float rh,
                                         const wchar_t* family, float size, int style, Color c) {
    if (!renderTarget) return;
    IDWriteTextFormat* fmt = getTextFormat(family, size, style, 1, 1);
    ComPtr<IDWriteTextLayout> layout;
    dwriteFactory->CreateTextLayout(text, (UINT32)wcslen(text), fmt, rw, rh, &layout);
    renderTarget->DrawTextLayout(D2D1::Point2F(x, y), layout.Get(), getBrush(c));
}

void Direct2DRenderer::drawTextLeft(const wchar_t* text, float x, float y, float rw, float rh,
                                     const wchar_t* family, float size, int style, Color c) {
    if (!renderTarget) return;
    IDWriteTextFormat* fmt = getTextFormat(family, size, style, 0, 1);
    ComPtr<IDWriteTextLayout> layout;
    dwriteFactory->CreateTextLayout(text, (UINT32)wcslen(text), fmt, rw, rh, &layout);
    renderTarget->DrawTextLayout(D2D1::Point2F(x, y), layout.Get(), getBrush(c));
}
