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
    // 1. 高 DPI 感知
    SetProcessDPIAware();

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
