#include "SoundManager.h"
#include <cmath>
#include <cstdlib>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static const int SAMPLE_RATE     = 44100;
static const int BITS_PER_SAMPLE = 16;
static const int NUM_CHANNELS    = 1;

// 健康阈值
static const DWORD HEALTH_TIMEOUT_MS     = 5000;   // 5 秒无回调 → 不健康
static const int   MAX_CONSECUTIVE_ERRORS = 5;     // 连续失败 → 重置设备
static const DWORD WRITE_TIMEOUT_MS       = 2000;  // 单次 write 超时（仅供参考）

// ========== WAV 构建 ==========

static void writeUint32(std::vector<BYTE>& buf, size_t off, DWORD v) {
    buf[off]=v&0xFF; buf[off+1]=(v>>8)&0xFF; buf[off+2]=(v>>16)&0xFF; buf[off+3]=(v>>24)&0xFF;
}
static void writeUint16(std::vector<BYTE>& buf, size_t off, WORD v) {
    buf[off]=v&0xFF; buf[off+1]=(v>>8)&0xFF;
}

static std::vector<BYTE> makeWav(const std::vector<short>& samples) {
    DWORD ds = (DWORD)(samples.size()*sizeof(short));
    std::vector<BYTE> wav(44+ds);
    wav[0]='R';wav[1]='I';wav[2]='F';wav[3]='F'; writeUint32(wav,4,36+ds);
    wav[8]='W';wav[9]='A';wav[10]='V';wav[11]='E';
    wav[12]='f';wav[13]='m';wav[14]='t';wav[15]=' '; writeUint32(wav,16,16);
    writeUint16(wav,20,1); writeUint16(wav,22,NUM_CHANNELS); writeUint32(wav,24,SAMPLE_RATE);
    writeUint32(wav,28,SAMPLE_RATE*NUM_CHANNELS*BITS_PER_SAMPLE/8);
    writeUint16(wav,32,(WORD)(NUM_CHANNELS*BITS_PER_SAMPLE/8)); writeUint16(wav,34,BITS_PER_SAMPLE);
    wav[36]='d';wav[37]='a';wav[38]='t';wav[39]='a'; writeUint32(wav,40,ds);
    memcpy(&wav[44],samples.data(),ds);
    return wav;
}

// ========== 音效合成 ==========

void SoundManager::generateShootWav() {
    const float d=0.06f; int n=(int)(SAMPLE_RATE*d); std::vector<short> s(n);
    for(int i=0;i<n;i++){float t=(float)i/SAMPLE_RATE,p=(float)i/n;
        float f=1200.0f*powf(150.0f/1200.0f,p), env=powf(1.0f-p,1.5f);
        float sm=sinf(2.0f*(float)M_PI*f*t)*0.7f+sinf(2.0f*(float)M_PI*f*2.0f*t)*0.2f+sinf(2.0f*(float)M_PI*f*0.5f*t)*0.1f;
        s[i]=(short)(sm*env*16000.0f);}
    m_shootWav=makeWav(s);
}
void SoundManager::generateHitWav() {
    const float d=0.10f; int n=(int)(SAMPLE_RATE*d); std::vector<short> s(n,0);
    for(int i=0;i<n;i++){float p=(float)i/n;
        float noise=((float)(rand()%20000-10000)/10000.0f);
        float ne=p<0.05f?1.0f:powf(1.0f-(p-0.05f)/0.95f,2.5f);
        float t=(float)i/SAMPLE_RATE, punch=sinf(2.0f*(float)M_PI*80.0f*t)*expf(-p*20.0f);
        s[i]=(short)((noise*ne*0.65f+punch*0.35f)*20000.0f);}
    m_hitWav=makeWav(s);
}
void SoundManager::generateExplosionWav() {
    const float d=0.30f; int n=(int)(SAMPLE_RATE*d); std::vector<short> s(n,0);
    for(int i=0;i<n;i++){float p=(float)i/n,t=(float)i/SAMPLE_RATE;
        float noise=((float)(rand()%20000-10000)/10000.0f),ne=powf(1.0f-p,1.2f);
        float rumble=sinf(2.0f*(float)M_PI*50.0f*t)*0.4f+sinf(2.0f*(float)M_PI*90.0f*t)*0.3f+sinf(2.0f*(float)M_PI*140.0f*t)*0.2f;
        float re=expf(-p*4.0f),debris=sinf(2.0f*(float)M_PI*(800.0f+p*3000.0f)*t)*0.08f*expf(-p*8.0f);
        s[i]=(short)((noise*ne*0.5f+rumble*re*0.4f+debris*0.1f)*24000.0f);}
    m_explosionWav=makeWav(s);
}

// ========== 单例 ==========

SoundManager& SoundManager::instance() { static SoundManager sm; return sm; }

// ========== 初始化 / 关闭 ==========

bool SoundManager::init() {
    if (m_initialized) return true;

    InitializeCriticalSection(&m_cs);
    generateShootWav(); generateHitWav(); generateExplosionWav();

    m_waveFmt.wFormatTag=WAVE_FORMAT_PCM; m_waveFmt.nChannels=NUM_CHANNELS;
    m_waveFmt.nSamplesPerSec=SAMPLE_RATE; m_waveFmt.wBitsPerSample=BITS_PER_SAMPLE;
    m_waveFmt.nBlockAlign=(WORD)(NUM_CHANNELS*BITS_PER_SAMPLE/8);
    m_waveFmt.nAvgBytesPerSec=SAMPLE_RATE*m_waveFmt.nBlockAlign;
    m_waveFmt.cbSize=0;

    MMRESULT res = waveOutOpen(&m_hWaveOut, WAVE_MAPPER, &m_waveFmt,
                                (DWORD_PTR)waveOutProc, (DWORD_PTR)this, CALLBACK_FUNCTION);
    if (res != MMSYSERR_NOERROR) {
        m_hWaveOut=nullptr; DeleteCriticalSection(&m_cs);
        m_initialized=true; return false;
    }

    m_lastCallbackTick = GetTickCount();
    m_lastWriteTick    = GetTickCount();
    m_consecutiveErrors = 0;
    m_deviceOk = true;

    m_hWakeEvent = CreateEventA(nullptr, FALSE, FALSE, nullptr);
    m_hDoneEvent = CreateEventA(nullptr, FALSE, FALSE, nullptr);
    m_hQuitEvent = CreateEventA(nullptr, TRUE,  FALSE, nullptr);
    m_queueHead = m_queueTail = 0;

    m_hAudioThread = CreateThread(nullptr, 0, audioThreadProc, this, 0, nullptr);
    if (!m_hAudioThread) {
        waveOutClose(m_hWaveOut); m_hWaveOut=nullptr;
        CloseHandle(m_hWakeEvent); CloseHandle(m_hDoneEvent); CloseHandle(m_hQuitEvent);
        m_hWakeEvent=m_hDoneEvent=m_hQuitEvent=nullptr;
        DeleteCriticalSection(&m_cs); m_initialized=true; return false;
    }

    m_initialized = true;
    return true;
}

void SoundManager::shutdown() {
    if (!m_initialized) return;

    if (m_hQuitEvent) SetEvent(m_hQuitEvent);
    if (m_hAudioThread) { WaitForSingleObject(m_hAudioThread, 3000); CloseHandle(m_hAudioThread); m_hAudioThread=nullptr; }

    if (m_hWaveOut) {
        waveOutReset(m_hWaveOut); Sleep(50);
        EnterCriticalSection(&m_cs);
        for (auto* h : m_activeHeaders) { waveOutUnprepareHeader(m_hWaveOut,h,sizeof(WAVEHDR)); delete[] h->lpData; delete h; }
        for (auto* h : m_doneHeaders)   { waveOutUnprepareHeader(m_hWaveOut,h,sizeof(WAVEHDR)); delete[] h->lpData; delete h; }
        m_activeHeaders.clear(); m_doneHeaders.clear();
        LeaveCriticalSection(&m_cs);
        waveOutClose(m_hWaveOut); m_hWaveOut=nullptr;
    }

    if (m_hWakeEvent) { CloseHandle(m_hWakeEvent); m_hWakeEvent=nullptr; }
    if (m_hDoneEvent) { CloseHandle(m_hDoneEvent); m_hDoneEvent=nullptr; }
    if (m_hQuitEvent) { CloseHandle(m_hQuitEvent); m_hQuitEvent=nullptr; }
    DeleteCriticalSection(&m_cs);
    m_initialized = false;
}

// ========== 音频线程 ==========

DWORD WINAPI SoundManager::audioThreadProc(LPVOID param) {
    SoundManager* self = (SoundManager*)param;
    // 三个事件：队列唤醒 / doneList 有数据 / 退出
    HANDLE ev[3] = { self->m_hWakeEvent, self->m_hDoneEvent, self->m_hQuitEvent };

    while (true) {
        // 超时 500ms，用于定期健康自检
        DWORD r = WaitForMultipleObjects(3, ev, FALSE, 500);

        if (r == WAIT_OBJECT_0 + 2) break; // 退出

        // ── 处理待写入队列 ──
        if (r == WAIT_OBJECT_0 || r == WAIT_TIMEOUT) {
            while (self->m_queueHead != self->m_queueTail) {
                LONG tail = self->m_queueTail;
                int type = self->m_queue[tail];
                self->m_queueTail = (tail + 1) % MAX_QUEUE;

                const std::vector<BYTE>* wav = nullptr;
                if (type == 0) wav = &self->m_shootWav;
                else if (type == 1) wav = &self->m_hitWav;
                else if (type == 2) wav = &self->m_explosionWav;
                if (wav) self->writeWav(*wav);
            }
        }

        // ── 清理已完成的 header（回调标记的） ──
        if (r == WAIT_OBJECT_0 + 1 || r == WAIT_TIMEOUT) {
            self->cleanupDoneHeaders();
        }

        // ── 健康自检（每次超时唤醒时执行） ──
        if (r == WAIT_TIMEOUT) {
            DWORD now = GetTickCount();
            DWORD callbackAge = now - self->m_lastCallbackTick;
            int errors = self->m_consecutiveErrors;

            // 连续错误过多 → 重置设备
            if (errors >= MAX_CONSECUTIVE_ERRORS) {
                self->m_deviceOk = false;
                // 通过重置恢复：关闭并重新打开
                EnterCriticalSection(&self->m_cs);
                HWAVEOUT hwo = self->m_hWaveOut;
                self->m_hWaveOut = nullptr;
                // 清理所有活跃 header
                for (auto* h : self->m_activeHeaders) { delete[] h->lpData; delete h; }
                for (auto* h : self->m_doneHeaders)   { delete[] h->lpData; delete h; }
                self->m_activeHeaders.clear(); self->m_doneHeaders.clear();
                LeaveCriticalSection(&self->m_cs);

                if (hwo) { waveOutReset(hwo); waveOutClose(hwo); }

                // 重新打开
                MMRESULT res = waveOutOpen(&self->m_hWaveOut, WAVE_MAPPER, &self->m_waveFmt,
                                            (DWORD_PTR)waveOutProc, (DWORD_PTR)self, CALLBACK_FUNCTION);
                if (res == MMSYSERR_NOERROR) {
                    self->m_deviceOk = true;
                    self->m_consecutiveErrors = 0;
                    self->m_lastCallbackTick = GetTickCount();
                    self->m_lastWriteTick = GetTickCount();
                }
            }
            // 长时间无回调 → 标记不健康（主线程可检测）
            else if (callbackAge > HEALTH_TIMEOUT_MS) {
                self->m_deviceOk = false;
            }
        }
    }
    return 0;
}

// ========== 主线程接口 ==========

void SoundManager::playShoot()     { playWav(m_shootWav); }
void SoundManager::playHit()       { playWav(m_hitWav); }
void SoundManager::playExplosion() { playWav(m_explosionWav); }

void SoundManager::playWav(const std::vector<BYTE>& wavData) {
    if (!m_initialized || !m_hAudioThread || wavData.empty()) return;

    int type = -1;
    if (&wavData == &m_shootWav)      type = 0;
    else if (&wavData == &m_hitWav)   type = 1;
    else if (&wavData == &m_explosionWav) type = 2;
    if (type < 0) return;

    LONG head = m_queueHead;
    LONG next = (head + 1) % MAX_QUEUE;
    if (next == m_queueTail) return;
    m_queue[head] = type;
    InterlockedExchange(&m_queueHead, next);
    SetEvent(m_hWakeEvent);
}

// ========== 健康检查（主线程调用） ==========

bool SoundManager::isAudioHealthy() const {
    if (!m_initialized) return true; // 未初始化视为健康
    if (!m_deviceOk) return false;
    DWORD age = GetTickCount() - m_lastCallbackTick;
    return (age < HEALTH_TIMEOUT_MS);
}

void SoundManager::resetAudioDevice() {
    if (!m_initialized) return;

    EnterCriticalSection(&m_cs);
    HWAVEOUT hwo = m_hWaveOut;
    m_hWaveOut = nullptr;
    for (auto* h : m_activeHeaders) { delete[] h->lpData; delete h; }
    for (auto* h : m_doneHeaders)   { delete[] h->lpData; delete h; }
    m_activeHeaders.clear(); m_doneHeaders.clear();
    LeaveCriticalSection(&m_cs);

    if (hwo) { waveOutReset(hwo); waveOutClose(hwo); }

    MMRESULT res = waveOutOpen(&m_hWaveOut, WAVE_MAPPER, &m_waveFmt,
                                (DWORD_PTR)waveOutProc, (DWORD_PTR)this, CALLBACK_FUNCTION);
    if (res == MMSYSERR_NOERROR) {
        m_deviceOk = true;
        m_consecutiveErrors = 0;
        m_lastCallbackTick = GetTickCount();
        m_lastWriteTick = GetTickCount();
    }
}

// ========== 在音频线程中执行 waveOut 写入 ==========

void SoundManager::writeWav(const std::vector<BYTE>& wavData) {
    if (!m_hWaveOut || wavData.empty()) return;

    EnterCriticalSection(&m_cs);
    if ((int)m_activeHeaders.size() >= MAX_ACTIVE_SOUNDS) {
        LeaveCriticalSection(&m_cs); return;
    }

    WAVEHDR* hdr = new WAVEHDR();
    memset(hdr, 0, sizeof(WAVEHDR));
    DWORD ds = (DWORD)wavData.size();
    char* dc = new char[ds];
    memcpy(dc, wavData.data(), ds);
    hdr->lpData = dc; hdr->dwBufferLength = ds;

    MMRESULT res = waveOutPrepareHeader(m_hWaveOut, hdr, sizeof(WAVEHDR));
    if (res != MMSYSERR_NOERROR) {
        LeaveCriticalSection(&m_cs); delete[] dc; delete hdr;
        InterlockedIncrement(&m_consecutiveErrors);
        return;
    }
    m_activeHeaders.push_back(hdr);
    LeaveCriticalSection(&m_cs);

    res = waveOutWrite(m_hWaveOut, hdr, sizeof(WAVEHDR));
    InterlockedExchange(&m_lastWriteTick, GetTickCount());

    if (res != MMSYSERR_NOERROR) {
        EnterCriticalSection(&m_cs);
        for (auto it=m_activeHeaders.begin(); it!=m_activeHeaders.end(); ++it) {
            if (*it==hdr) { m_activeHeaders.erase(it); break; }
        }
        LeaveCriticalSection(&m_cs);
        waveOutUnprepareHeader(m_hWaveOut, hdr, sizeof(WAVEHDR));
        delete[] dc; delete hdr;
        InterlockedIncrement(&m_consecutiveErrors);
    } else {
        InterlockedExchange(&m_consecutiveErrors, 0);  // 成功则清零错误计数
    }
}

// ========== 清理已完成的 header（在音频线程中执行） ==========

void SoundManager::cleanupDoneHeaders() {
    EnterCriticalSection(&m_cs);
    if (m_doneHeaders.empty()) { LeaveCriticalSection(&m_cs); return; }

    // 取走所有 done header（避免长时间持锁）
    std::vector<WAVEHDR*> done;
    done.swap(m_doneHeaders);
    LeaveCriticalSection(&m_cs);

    for (auto* hdr : done) {
        if (m_hWaveOut) {
            waveOutUnprepareHeader(m_hWaveOut, hdr, sizeof(WAVEHDR));
        }
        delete[] hdr->lpData;
        delete hdr;
    }
    // 成功清理 → 更新健康时间戳
    InterlockedExchange(&m_lastCallbackTick, GetTickCount());
}

// ========== waveOut 回调（运行在驱动线程） ==========
// ★ 关键：回调中绝不调用 waveOutUnprepareHeader（会触发驱动内部锁）
//   仅将 header 移入 doneList，由音频线程统一清理

void CALLBACK SoundManager::waveOutProc(HWAVEOUT /*hwo*/, UINT uMsg,
                                         DWORD_PTR dwInstance, DWORD_PTR dwParam1,
                                         DWORD_PTR /*dwParam2*/) {
    if (uMsg != WOM_DONE) return;

    SoundManager* self = (SoundManager*)dwInstance;
    WAVEHDR* hdr = (WAVEHDR*)dwParam1;
    if (!hdr) return;

    EnterCriticalSection(&self->m_cs);
    // 从活跃列表移到完成列表
    auto& active = self->m_activeHeaders;
    for (auto it = active.begin(); it != active.end(); ++it) {
        if (*it == hdr) {
            active.erase(it);
            self->m_doneHeaders.push_back(hdr);
            break;
        }
    }
    LeaveCriticalSection(&self->m_cs);

    // 更新时间戳 + 唤醒音频线程处理 doneList
    InterlockedExchange(&self->m_lastCallbackTick, GetTickCount());
    SetEvent(self->m_hDoneEvent);
}
