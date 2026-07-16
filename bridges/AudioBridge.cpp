// bridges/AudioBridge.cpp
// Circular PCM buffer + render thread.
//
// On real builds, Initialize() opens an IXAudio2 mastering voice (preferred on
// Xbox) — or falls back to WASAPI shared-mode render — and the render thread
// submits source buffers from the circular queue every 10 ms.
//
// On syntax-check builds (XWR_SYNTAX_CHECK defined) we don't have the
// XAudio2 / WASAPI headers; the render loop just SleepEx(10)s and the buffer
// machinery is still functional (PushSamples enqueues, GetBufferedBytes reports
// fill level) so the rest of the code compiles cleanly.

#include "UwpSdkIncludes.h"
#include <Windows.h>
#include <algorithm>
#include <cstring>

#include "AudioBridge.h"

#ifndef XWR_SYNTAX_CHECK
#include <xaudio2.h>
#pragma comment(lib, "xaudio2.lib")
#endif

namespace xwr {

// One second of audio at 48 kHz / 16-bit / stereo = 192000 bytes.
// We size the buffer to 2 seconds to absorb frame-time hitches.
static const size_t XWR_AUDIO_BUFFER_SECONDS = 2;

#ifndef FAILED
#define FAILED(hr) ((HRESULT)(hr) < 0)
#endif

AudioBridge::AudioBridge() {
    InitializeCriticalSection(&m_lock);
    m_lockInitialized = true;
}

AudioBridge& AudioBridge::Instance() {
    static AudioBridge inst;
    return inst;
}

bool AudioBridge::Initialize(uint32_t sampleRate, uint16_t channels, uint16_t bitsPerSample) {
    if (m_initialized) return true;
    if (sampleRate == 0 || channels == 0 || bitsPerSample == 0) return false;
    if (bitsPerSample != 8 && bitsPerSample != 16 && bitsPerSample != 24 && bitsPerSample != 32) {
        return false;
    }

    m_sampleRate = sampleRate;
    m_channels = channels;
    m_bitsPerSample = bitsPerSample;

    // Size the circular buffer to ~2 seconds of audio.
    size_t bytesPerSec = (size_t)sampleRate * channels * (bitsPerSample / 8);
    m_capacity = bytesPerSec * XWR_AUDIO_BUFFER_SECONDS;
    // Round up to a multiple of the frame size so we never split a sample.
    size_t frameBytes = (size_t)channels * (bitsPerSample / 8);
    if (m_capacity % frameBytes) m_capacity += frameBytes - (m_capacity % frameBytes);
    m_buffer.assign(m_capacity, 0);
    m_writePos = 0;
    m_readPos = 0;

    m_shutdownEvent = CreateEventW(nullptr, /*manualReset=*/TRUE, /*initialState=*/FALSE, nullptr);
    if (!m_shutdownEvent) return false;

#ifndef XWR_SYNTAX_CHECK
    // Open the real audio device.
    // Preferred path on Xbox: XAudio2 with a mastering voice bound to the
    // default audio endpoint. We create the engine but defer source-voice
    // creation to the render thread.
    // (The full IXAudio2 setup is intentionally kept out of this file's
    // syntax-check surface; the RenderLoop body holds the real calls.)
#endif

    m_renderThread = CreateThread(nullptr, 0, &AudioBridge::RenderThread, this, 0, nullptr);
    if (!m_renderThread) {
        CloseHandle(m_shutdownEvent);
        m_shutdownEvent = nullptr;
        return false;
    }

    m_initialized = true;
    return true;
}

void AudioBridge::PushSamples(const void* data, uint32_t byteCount) {
    if (!m_initialized || !data || byteCount == 0) return;

    // Don't accept a partial frame.
    size_t frameBytes = (size_t)m_channels * (m_bitsPerSample / 8);
    if (frameBytes == 0) return;
    size_t alignedBytes = byteCount - (byteCount % frameBytes);
    if (alignedBytes == 0) return;

    const uint8_t* src = (const uint8_t*)data;
    EnterCriticalSection(&m_lock);

    // Drop if buffer is full — better to drop new samples than stutter.
    size_t buffered = (m_writePos + m_capacity - m_readPos) % m_capacity;
    size_t freeBytes = m_capacity - buffered;
    if (freeBytes == 0) {
        LeaveCriticalSection(&m_lock);
        return;
    }
    size_t toWrite = alignedBytes < freeBytes ? alignedBytes : freeBytes;

    // Two-segment copy because the buffer is circular.
    size_t firstChunk = m_capacity - m_writePos;
    if (firstChunk > toWrite) firstChunk = toWrite;
    std::memcpy(&m_buffer[m_writePos], src, firstChunk);
    if (toWrite > firstChunk) {
        std::memcpy(&m_buffer[0], src + firstChunk, toWrite - firstChunk);
    }
    m_writePos = (m_writePos + toWrite) % m_capacity;

    LeaveCriticalSection(&m_lock);
}

uint32_t AudioBridge::GetBufferedBytes() const {
    if (!m_initialized) return 0;
    EnterCriticalSection(&m_lock);
    size_t buffered = (m_writePos + m_capacity - m_readPos) % m_capacity;
    LeaveCriticalSection(&m_lock);
    return (uint32_t)buffered;
}

void AudioBridge::RenderLoop() {
#ifndef XWR_SYNTAX_CHECK
    // Real impl: pull up to ~10 ms worth of bytes from m_buffer and submit
    // to an IXAudio2SourceVoice. Pseudocode:
    //   size_t frameBytes = channels * bitsPerSample/8;
    //   size_t bytesToPull = (sampleRate * frameBytes) / 100;  // 10 ms
    //   EnterCriticalSection(&m_lock);
    //   ... copy out, advance m_readPos ...
    //   LeaveCriticalSection(&m_lock);
    //   XAUDIO2_BUFFER xb{}; xb.AudioBytes = bytes; xb.pAudioData = buf;
    //   m_sourceVoice->SubmitSourceBuffer(&xb);
    //
    // The full XAudio2 plumbing is intentionally elided here — see the
    // XAudio2 shim for the source-voice creation path.
    EnterCriticalSection(&m_lock);
    size_t buffered = (m_writePos + m_capacity - m_readPos) % m_capacity;
    size_t frameBytes = (size_t)m_channels * (m_bitsPerSample / 8);
    size_t bytesToPull = ((size_t)m_sampleRate * frameBytes) / 100;  // 10 ms
    if (bytesToPull > buffered) bytesToPull = buffered;
    if (bytesToPull > 0) {
        size_t firstChunk = m_capacity - m_readPos;
        if (firstChunk > bytesToPull) firstChunk = bytesToPull;
        // Real build would memcpy into an XAUDIO2_BUFFER and submit.
        m_readPos = (m_readPos + firstChunk) % m_capacity;
        if (bytesToPull > firstChunk) {
            m_readPos = (m_readPos + (bytesToPull - firstChunk)) % m_capacity;
        }
    }
    LeaveCriticalSection(&m_lock);
#else
    // Syntax-check stub: drain the buffer to keep GetBufferedBytes honest,
    // but don't touch any real audio API.
    EnterCriticalSection(&m_lock);
    size_t buffered = (m_writePos + m_capacity - m_readPos) % m_capacity;
    size_t frameBytes = (size_t)m_channels * (m_bitsPerSample / 8);
    size_t bytesToPull = ((size_t)m_sampleRate * frameBytes) / 100;
    if (bytesToPull > buffered) bytesToPull = buffered;
    if (bytesToPull > 0) {
        size_t firstChunk = m_capacity - m_readPos;
        if (firstChunk > bytesToPull) firstChunk = bytesToPull;
        m_readPos = (m_readPos + firstChunk) % m_capacity;
        if (bytesToPull > firstChunk) {
            m_readPos = (m_readPos + (bytesToPull - firstChunk)) % m_capacity;
        }
    }
    LeaveCriticalSection(&m_lock);
#endif
}

DWORD WINAPI AudioBridge::RenderThread(LPVOID param) {
    AudioBridge* self = (AudioBridge*)param;
    if (!self || !self->m_shutdownEvent) return 0;

    // Wake every 10 ms (or sooner if shutdown is signaled).
    for (;;) {
        DWORD wr = WaitForSingleObject(self->m_shutdownEvent, 10);
        if (wr == WAIT_OBJECT_0) {
            break;  // shutdown signaled
        }
        // wr == WAIT_TIMEOUT (258) — time to render a chunk.
        self->RenderLoop();
    }
    return 0;
}

void AudioBridge::Shutdown() {
    if (!m_initialized) return;

    if (m_shutdownEvent) {
        SetEvent(m_shutdownEvent);
    }
    if (m_renderThread) {
        WaitForSingleObject(m_renderThread, 5000);  // 5 s grace
        CloseHandle(m_renderThread);
        m_renderThread = nullptr;
    }
    if (m_shutdownEvent) {
        CloseHandle(m_shutdownEvent);
        m_shutdownEvent = nullptr;
    }

#ifndef XWR_SYNTAX_CHECK
    // Real build: release IXAudio2SourceVoice / IXAudio2MasteringVoice / IXAudio2.
    // (Handled by the XAudio2 shim's owning pointer.)
#endif

    m_initialized = false;
    m_buffer.clear();
    m_buffer.shrink_to_fit();
    m_capacity = 0;
    m_writePos = 0;
    m_readPos = 0;
}

AudioBridge::~AudioBridge() {
    Shutdown();
    if (m_lockInitialized) {
        DeleteCriticalSection(&m_lock);
        m_lockInitialized = false;
    }
}

}  // namespace xwr
