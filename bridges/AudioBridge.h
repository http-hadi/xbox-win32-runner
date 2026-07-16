// bridges/AudioBridge.h
// Circular PCM audio buffer + render thread that connects to the WASAPI /
// XAudio2 endpoint on real builds.
//
// Game side (via the winmm waveOut shim or xaudio2 source-voice shim) calls
// PushSamples() with raw PCM bytes. A background render thread wakes every
// 10 ms and pulls from the buffer, submitting to the real audio device.
//
// The buffer is lock-free on the read side (single render thread) and
// guarded by a CRITICAL_SECTION on the write side (multiple game threads
// may push).

#pragma once
#include <Windows.h>

#include <cstdint>
#include <vector>

namespace xwr {

class AudioBridge {
public:
    static AudioBridge& Instance();

    ~AudioBridge();

    // Initialize with sample rate, channels, bits per sample.
    // Spawns the render thread. Returns false if the audio device can't be
    // opened (in that case PushSamples becomes a no-op drop sink).
    bool Initialize(uint32_t sampleRate, uint16_t channels, uint16_t bitsPerSample);

    // Push PCM samples into the circular buffer.
    // byteCount must be a multiple of (channels * bitsPerSample/8).
    void PushSamples(const void* data, uint32_t byteCount);

    // Get current buffer fill level (bytes).
    uint32_t GetBufferedBytes() const;

    void Shutdown();

private:
    AudioBridge();
    AudioBridge(const AudioBridge&) = delete;
    AudioBridge& operator=(const AudioBridge&) = delete;

    static DWORD WINAPI RenderThread(LPVOID param);
    void RenderLoop();

    // Circular buffer: capacity is fixed at Initialize time to ~1 second of audio.
    std::vector<uint8_t> m_buffer;
    size_t m_writePos = 0;
    size_t m_readPos = 0;
    size_t m_capacity = 0;

    mutable CRITICAL_SECTION m_lock;
    bool m_lockInitialized = false;

    HANDLE m_renderThread = nullptr;
    HANDLE m_shutdownEvent = nullptr;

    uint32_t m_sampleRate = 48000;
    uint16_t m_channels = 2;
    uint16_t m_bitsPerSample = 16;
    bool m_initialized = false;
};

}  // namespace xwr
