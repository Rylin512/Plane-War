// ============================================================
// PlaneWar Watchdog — 独立进程，监控主游戏是否卡死
// 用法: watchdog.exe [PlaneWar.exe路径]
// 通过共享内存 "PlaneWar_Heartbeat" 监测心跳，
// 超过 3 秒无心跳则生成 minidump + 日志，然后终止游戏。
// ============================================================

#include <windows.h>
#include <tlhelp32.h>
#include <dbghelp.h>
#include <cstdio>
#include <ctime>

#pragma comment(lib, "dbghelp.lib")

// ── 与 Game.h 中 HeartbeatData 保持一致 ──
struct HeartbeatData {
    volatile LONG frameCount;
    volatile LONG lastTick;
    DWORD processId;
    DWORD gameState;
    DWORD currentLevel;
    DWORD score;
    DWORD enemyCount;
    DWORD bulletCount;
    DWORD itemCount;
    DWORD godMode;
    DWORD oneHitKill;
    float currentFPS;
    DWORD resolutionW;
    DWORD resolutionH;
    char  reserved[32];
};

static const char* gameStateName(DWORD s) {
    switch (s) {
    case 0: return "MENU";      case 1: return "SETTINGS";
    case 2: return "PLAYING";   case 3: return "PAUSED";
    case 4: return "LEVEL_TRANSITION"; case 5: return "GAME_OVER";
    case 6: return "VICTORY";   case 7: return "LEADERBOARD";
    case 8: return "NAME_ENTRY"; case 9: return "HELP";
    case 10: return "SKIN_SELECT"; case 11: return "CHEAT_MENU";
    default: return "UNKNOWN";
    }
}

// ── 获取日志文件路径（与游戏 exe 同目录，正常扩展名） ──
static void getLogPath(char* buf, size_t bufSize, const char* gamePath,
                       const char* tag, const char* ext) {
    // 1. 分离目录和文件名
    char dir[MAX_PATH] = {};
    char name[MAX_PATH] = {};
    const char* sl = strrchr(gamePath, '\\');
    if (sl) {
        // 有路径：C:\path\PlaneWar.exe
        size_t dirLen = sl - gamePath + 1;
        memcpy(dir, gamePath, dirLen);
        strncpy_s(name, sizeof(name), sl + 1, _TRUNCATE);
    } else {
        // 无路径：PlaneWar.exe → 当前目录
        dir[0] = '.'; dir[1] = '\\'; dir[2] = '\0';
        strncpy_s(name, sizeof(name), gamePath, _TRUNCATE);
    }
    // 2. 去掉扩展名
    char* dot = strrchr(name, '.');
    if (dot) *dot = '\0';

    // 3. 时间戳
    SYSTEMTIME st;
    GetLocalTime(&st);
    char ts[32];
    sprintf_s(ts, sizeof(ts), "%04d%02d%02d_%02d%02d%02d",
              st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

    // 4. 拼接：dir\name_tag_timestamp.ext
    sprintf_s(buf, bufSize, "%s%s_%s_%s.%s", dir, name, tag, ts, ext);
}

// ── 生成 MiniDump ──
static void createMiniDump(HANDLE hProcess, DWORD pid, const char* basePath) {
    char dumpPath[MAX_PATH];
    getLogPath(dumpPath, sizeof(dumpPath), basePath, "hangdump", "dmp");

    HANDLE hFile = CreateFileA(dumpPath, GENERIC_WRITE, 0, nullptr,
                                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("  [WARN] Cannot create dump file: %s\n", dumpPath);
        return;
    }

    BOOL ok = MiniDumpWriteDump(hProcess, pid, hFile,
                                 (MINIDUMP_TYPE)(MiniDumpWithFullMemory |
                                                 MiniDumpWithHandleData |
                                                 MiniDumpWithThreadInfo |
                                                 MiniDumpWithUnloadedModules),
                                 nullptr, nullptr, nullptr);
    CloseHandle(hFile);
    if (ok)
        printf("  MiniDump saved: %s\n", dumpPath);
    else
        printf("  [WARN] MiniDumpWriteDump failed (error %d)\n", GetLastError());
}

// ── 写入挂死日志 ──
static void writeHangLog(const char* basePath, HeartbeatData* hb) {
    char logPath[MAX_PATH];
    getLogPath(logPath, sizeof(logPath), basePath, "hangreport", "log");

    FILE* f = nullptr;
    fopen_s(&f, logPath, "w");
    if (!f) return;

    SYSTEMTIME st;
    GetLocalTime(&st);

    fprintf(f, "============================================\n");
    fprintf(f, "  Plane War - Hang Detection Report\n");
    fprintf(f, "============================================\n");
    fprintf(f, "Detected: %04d-%02d-%02d %02d:%02d:%02d\n\n",
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

    fprintf(f, "[Heartbeat Status]\n");
    fprintf(f, "  Process ID:    %d\n", hb->processId);
    fprintf(f, "  Last Frame:    %d\n", hb->frameCount);
    fprintf(f, "  Last Tick:     %d (current: %d, diff: %d ms)\n",
            hb->lastTick, GetTickCount(), GetTickCount() - hb->lastTick);
    fprintf(f, "  FPS:           %.1f\n", hb->currentFPS);

    fprintf(f, "\n[Game State at Freeze]\n");
    fprintf(f, "  State:         %s (%d)\n", gameStateName(hb->gameState), hb->gameState);
    fprintf(f, "  Level:         %d\n", hb->currentLevel);
    fprintf(f, "  Score:         %d\n", hb->score);
    fprintf(f, "  Resolution:    %dx%d\n", hb->resolutionW, hb->resolutionH);
    fprintf(f, "  Enemies:       %d\n", hb->enemyCount);
    fprintf(f, "  Bullets:       %d\n", hb->bulletCount);
    fprintf(f, "  Items:         %d\n", hb->itemCount);
    fprintf(f, "  GodMode:       %s\n", hb->godMode ? "ON" : "OFF");
    fprintf(f, "  OneHitKill:    %s\n", hb->oneHitKill ? "ON" : "OFF");

    fprintf(f, "\n[Possible Causes]\n");
    fprintf(f, "  - Infinite loop in logic (update/collide)\n");
    fprintf(f, "  - Deadlock in audio thread (SoundManager)\n");
    fprintf(f, "  - GDI+/D2D driver hang in render\n");
    fprintf(f, "  - Mutex/CriticalSection deadlock\n");
    fprintf(f, "  - Blocking system call (file I/O, etc.)\n");

    fprintf(f, "\n============================================\n");
    fclose(f);
    printf("  Hang report saved: %s\n", logPath);
}

// ── 主入口 ──
int main(int argc, char* argv[]) {
    printf("============================================\n");
    printf("  Plane War Watchdog v1.0\n");
    printf("============================================\n\n");

    const char* gamePath = (argc > 1) ? argv[1] : "PlaneWar.exe";

    // 1. 启动游戏进程
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};
    char cmdLine[MAX_PATH];
    sprintf_s(cmdLine, sizeof(cmdLine), "\"%s\"", gamePath);

    printf("[1] Launching: %s\n", cmdLine);
    if (!CreateProcessA(nullptr, cmdLine, nullptr, nullptr, FALSE,
                        0, nullptr, nullptr, &si, &pi)) {
        printf("ERROR: Cannot launch %s (error %d)\n", gamePath, GetLastError());
        printf("Usage: watchdog.exe [path/to/PlaneWar.exe]\n");
        return 1;
    }
    printf("     PID: %d\n\n", pi.dwProcessId);

    // 2. 等待共享内存出现
    printf("[2] Waiting for heartbeat...\n");
    HANDLE hMap = nullptr;
    HeartbeatData* pHb = nullptr;
    for (int retry = 0; retry < 30; retry++) {
        hMap = OpenFileMappingA(FILE_MAP_READ, FALSE, "PlaneWar_Heartbeat");
        if (hMap) {
            pHb = (HeartbeatData*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, sizeof(HeartbeatData));
            if (pHb && pHb->processId == pi.dwProcessId) break;
            if (pHb) { UnmapViewOfFile(pHb); pHb = nullptr; }
            CloseHandle(hMap); hMap = nullptr;
        }
        Sleep(500);
    }
    if (!pHb) {
        printf("ERROR: Heartbeat not detected after 15s. Aborting.\n");
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
        return 1;
    }
    printf("     Heartbeat connected.\n\n");

    // 3. 监控循环
    printf("[3] Monitoring (freeze threshold: 3 seconds)...\n");
    LONG lastFrame = pHb->frameCount;
    DWORD lastChangeTick = GetTickCount();

    while (true) {
        Sleep(500);

        // 检查进程是否还在运行
        DWORD exitCode;
        if (!GetExitCodeProcess(pi.hProcess, &exitCode) || exitCode != STILL_ACTIVE) {
            printf("\n     Game process exited normally (code %d).\n", exitCode);
            break;
        }

        LONG curFrame = pHb->frameCount;
        DWORD nowTick = GetTickCount();
        DWORD elapsedSinceChange = nowTick - lastChangeTick;

        if (curFrame != lastFrame) {
            lastFrame = curFrame;
            lastChangeTick = nowTick;
            // 每 5 秒输出一次状态
            static DWORD lastStatus = 0;
            if (nowTick - lastStatus > 5000) {
                printf("     [OK] Frame=%d  FPS=%.1f  State=%s  Enemies=%d\n",
                       curFrame, pHb->currentFPS, gameStateName(pHb->gameState), pHb->enemyCount);
                lastStatus = nowTick;
            }
        }

        // 超过 3 秒无心跳 → 判定为卡死
        if (elapsedSinceChange > 3000) {
            printf("\n============================================\n");
            printf("!!! GAME FREEZE DETECTED !!!\n");
            printf("  Last frame: %d\n", lastFrame);
            printf("  Frozen for: %d ms\n", elapsedSinceChange);
            printf("  Game state: %s (%d)\n", gameStateName(pHb->gameState), pHb->gameState);
            printf("============================================\n\n");

            // 4. 生成诊断文件
            printf("[4] Generating diagnostics...\n");
            writeHangLog(gamePath, pHb);
            createMiniDump(pi.hProcess, pi.dwProcessId, gamePath);

            // 5. 终止卡死的游戏
            printf("\n[5] Terminating frozen game process...\n");
            TerminateProcess(pi.hProcess, 1);
            WaitForSingleObject(pi.hProcess, 5000);

            printf("\n============================================\n");
            printf("  Watchdog complete.\n");
            printf("  Check PlaneWar_hangreport_*.log and PlaneWar_hangdump_*.dmp\n");
            printf("============================================\n");
            break;
        }
    }

    // 清理
    if (pHb) UnmapViewOfFile(pHb);
    if (hMap) CloseHandle(hMap);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return 0;
}
