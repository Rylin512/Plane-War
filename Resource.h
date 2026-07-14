#pragma once
#include <windows.h>
#include <gdiplus.h>
#include <cwchar>

// ============================================================
// 文件：Resource.h
// 功能：资源管理 — 从 custom 文件夹加载素材 (纯 C，无 std::wstring)
// ============================================================

class Resource {
public:
    // ── 工具：路径拼接 ──
    static void MakePath(wchar_t* out, int outSize, const wchar_t* relative) {
        GetModuleFileNameW(nullptr, out, outSize);
        wchar_t* slash = wcsrchr(out, L'\\');
        if (slash) *(slash + 1) = L'\0'; else out[0] = L'\0';
        wcscat(out, L"custom\\");
        wcscat(out, relative);
    }

    // ── 加载图片 ──
    static Gdiplus::Image* LoadImage(const wchar_t* relative) {
        wchar_t full[MAX_PATH];
        MakePath(full, MAX_PATH, relative);
        Gdiplus::Image* img = Gdiplus::Image::FromFile(full);
        if (img && img->GetLastStatus() == Gdiplus::Ok) return img;
        delete img;
        return nullptr;
    }

    // ── 播放 BGM ──
    static void PlayBGM(const wchar_t* relative) {
        wchar_t full[MAX_PATH];
        MakePath(full, MAX_PATH, relative);
        wchar_t cmd[1024];
        _snwprintf(cmd, 1024, L"open \"%s\" type mpegvideo alias BGM", full);
        mciSendStringW(cmd, nullptr, 0, nullptr);
        mciSendStringW(L"play BGM repeat", nullptr, 0, nullptr);
    }

    static void StopBGM() {
        mciSendStringW(L"close BGM", nullptr, 0, nullptr);
    }

    // ── 预定义素材路径 ──
    static constexpr const wchar_t* BG_CIRCUIT = L"backgrounds\\circuit\\bg.png";
    static constexpr const wchar_t* BG_TYPE    = L"backings\\type\\bg.png";
    static constexpr const wchar_t* MUSIC_BGM  = L"music\\windowframe\\music.ogg";
    static constexpr const wchar_t* UI_FRAME   = L"window_themes\\pixel\\frame.png";
};
