#pragma once
#include <windows.h>
#include <gdiplus.h>

// ============================================================
// Resource.h — 素材加载工具（静态方法）
// ============================================================

class Resource {
public:
    // 图片资源 ID
    enum ImageID { BG_CIRCUIT, UI_FRAME };

    // 加载图片（返回 nullptr 表示文件不存在）
    static Gdiplus::Image* LoadImage(ImageID id);

    // 背景音乐
    static void PlayBGM(const wchar_t* path);
    static void StopBGM();
};
