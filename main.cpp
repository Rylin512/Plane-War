// ============================================================
// 文件：main.cpp
// 功能：程序入口 — 飞机大战游戏
// ============================================================

#include <windows.h>
#include "Game.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
    Game game;

    if (!game.init(hInstance)) {
        MessageBoxA(nullptr, "游戏初始化失败！", "错误", MB_OK | MB_ICONERROR);
        return 1;
    }

    game.run();
    game.shutdown();

    return 0;
}
