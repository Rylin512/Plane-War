#pragma once
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#include <vector>

// ============================================================
// SoundManager.h — 动态音效管理器（音频线程架构 v2）
//
// 架构：
//   主线程  ──(无锁队列)──▶  音频线程  ──(waveOut)──▶  驱动
//                              ▲
//   驱动回调 ──(仅标记done)──▶  doneList  ──(清理)──▶  音频线程
//
// 安全设计：
//   - 主线程永不调用 waveOut API（零阻塞）
//   - 回调永不调用 waveOutUnprepareHeader（避免驱动线程内部锁）
//   - 音频线程统一处理写入 + 清理，带超时自恢复
// ============================================================

class SoundManager {
public:
    static SoundManager& instance();

    bool init();
    void shutdown();

    // 音效播放（主线程调用，纯异步，永不阻塞）
    void playShoot();
    void playHit();
    void playExplosion();

    // 健康检查（主线程每帧调用，检测音频线程是否卡死）
    bool isAudioHealthy() const;
    void resetAudioDevice();  // 紧急恢复：重置 waveOut 设备

private:
    SoundManager() = default;
    ~SoundManager() { shutdown(); }
    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;

    // 合成
    void generateShootWav();
    void generateHitWav();
    void generateExplosionWav();

    // 队列操作
    void playWav(const std::vector<BYTE>& wavData);
    void writeWav(const std::vector<BYTE>& wavData);
    void cleanupDoneHeaders();

    // 线程 + 回调
    static void CALLBACK waveOutProc(HWAVEOUT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR);
    static DWORD WINAPI audioThreadProc(LPVOID);

    // ── 设备 ──
    HWAVEOUT     m_hWaveOut = nullptr;
    WAVEFORMATEX m_waveFmt = {};

    // ── 预合成数据 ──
    std::vector<BYTE> m_shootWav;
    std::vector<BYTE> m_hitWav;
    std::vector<BYTE> m_explosionWav;

    // ── 活跃 header + done 列表（受 m_cs 保护） ──
    CRITICAL_SECTION  m_cs;
    std::vector<WAVEHDR*> m_activeHeaders;
    std::vector<WAVEHDR*> m_doneHeaders;     // 回调标记完成，音频线程清理
    static const int MAX_ACTIVE_SOUNDS = 6;  // 降低并发数

    // ── 音频线程 ──
    HANDLE m_hAudioThread = nullptr;
    HANDLE m_hWakeEvent  = nullptr;  // 队列非空
    HANDLE m_hQuitEvent  = nullptr;  // 退出
    HANDLE m_hDoneEvent  = nullptr;  // doneList 非空（自动重置）

    // 无锁环形队列
    static const int MAX_QUEUE = 32;
    int  m_queue[MAX_QUEUE];
    LONG m_queueHead = 0;
    LONG m_queueTail = 0;

    // ── 健康追踪 ──
    volatile LONG m_lastCallbackTick = 0;   // 最后一次 WOM_DONE 回调时间
    volatile LONG m_lastWriteTick    = 0;   // 最后一次 waveOutWrite 完成时间
    volatile LONG m_consecutiveErrors = 0;  // 连续错误计数
    volatile bool m_deviceOk         = true; // 设备是否健康

    bool m_initialized = false;
};
