// ============================================================
// 文件：main.cpp
// 功能：入口 — GDI+ 初始化 + 高 DPI + 高精度时钟 + 素材加载
// ============================================================

#include <windows.h>
#include <gdiplus.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#include "Game.h"
#include "Resource.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    // 1. 高 DPI 感知 — 按优先级尝试 V2 → PerMonitor → System
    {
        // 尝试 Windows 10 1703+ Per-Monitor V2（最佳）
        HMODULE hUser32 = LoadLibraryA("user32.dll");
        if (hUser32) {
            typedef BOOL (WINAPI *SetDpiAwarenessContext_t)(HANDLE);
            auto fn = (SetDpiAwarenessContext_t)GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");
            // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 = -4
            if (fn && fn((HANDLE)(INT_PTR)(-4))) {
                FreeLibrary(hUser32);
                goto dpi_done;
            }
            FreeLibrary(hUser32);
        }
        // 尝试 Windows 8.1+ Per-Monitor DPI
        {
            HMODULE hShcore = LoadLibraryA("shcore.dll");
            if (hShcore) {
                typedef HRESULT (WINAPI *SetDpiAwareness_t)(int);
                auto fn = (SetDpiAwareness_t)GetProcAddress(hShcore, "SetProcessDpiAwareness");
                if (fn) fn(2);  // PROCESS_PER_MONITOR_DPI_AWARE = 2
                FreeLibrary(hShcore);
                if (fn) goto dpi_done;
            }
        }
        // 回退：Vista/7 系统级 DPI 感知
        SetProcessDPIAware();
    }
    dpi_done: ;

    // 2. 高精度时钟
    timeBeginPeriod(1);

    // 3. GDI+ 初始化
    Gdiplus::GdiplusStartupInput gdiInput;
    ULONG_PTR gdiToken;
    if (Gdiplus::GdiplusStartup(&gdiToken, &gdiInput, nullptr) != Gdiplus::Ok) {
        timeEndPeriod(1);
        MessageBoxA(nullptr, "GDI+ init failed!", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // 4. 随机种子
    srand((unsigned int)time(nullptr));

    // 5. 可选：播放背景音乐
    // Resource::PlayBGM(Resource::MUSIC_BGM);

    // 6. 启动游戏
    Game game;
    if (!game.init(hInstance)) {
        Gdiplus::GdiplusShutdown(gdiToken);
        timeEndPeriod(1);
        MessageBoxA(nullptr, "Game init failed!", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    game.run();
    game.shutdown();

    // 7. 清理
    Resource::StopBGM();
    Gdiplus::GdiplusShutdown(gdiToken);
    timeEndPeriod(1);

    return 0;
}
