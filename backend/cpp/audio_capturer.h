#pragma once

#include <mmdeviceapi.h>
#include <audioclient.h>

#include <vector>

#include <fstream>
#include <iostream>
#include <atomic>
#include <mutex>
#include <thread>

class AudioCapturer {
public:
    /**
    * @brief Captures audio from the default playback device (loopback capture)
    * @param secondsPerFile Duration of each captured audio file in seconds
    *
    * This class records audio being played through the default audio output device
    * and saves it to WAV files of specified duration inside "C:\\live-furigana\\audios".
    */
    static void startAudioCapture(int secondsPerFile = 1);
    static void stopAudioCapture();
    static bool isRecording();

private:
    static constexpr int VAD_FRAME_MS = 20;
    static constexpr float VAD_ENERGY_THRESHOLD = 0.008f;

    static constexpr int VAD_MIN_SPEECH_FRAMES = 12;  // ~240ms
    static constexpr int VAD_MIN_SILENCE_FRAMES = 18; // ~360ms
    static constexpr int HANGOVER_MAX = 10; // ~200ms

    static constexpr float VOLUME_MULTIPLIER = 0.9f;

    static std::atomic<bool> recording;
    static std::thread captureThread;
    static std::mutex recordMutex;
    static std::vector<BYTE> fullRecordingData;

    static void captureLoop(int secondsPerFile);
    static std::string getCurrentDateString();
    static void writeWavHeader(std::ofstream& out, int sampleRate, int bitsPerSample, int channels, size_t dataSize);
    static bool initializeAudioDevices(IMMDeviceEnumerator** pEnumerator,
        IMMDevice** pDevice,
        IAudioClient** pAudioClient,
        IAudioCaptureClient** pCaptureClient);
    static void processAudioBuffer(IAudioCaptureClient* pCaptureClient, int blockAlign, std::vector<BYTE>& audioData);
    static void saveSegmentedAudioFile(const std::vector<BYTE>& audioData, WAVEFORMATEX* pwfx, int segmentIdx, const std::string& dateStr);
    static void saveFullAudioFile(const std::vector<BYTE>& audioData, WAVEFORMATEX* pwfx, const std::string& dateStr);
    static void cleanupAudioDevices(WAVEFORMATEX* pwfx,
        IAudioCaptureClient* pCaptureClient,
        IAudioClient* pAudioClient,
        IMMDevice* pDevice,
        IMMDeviceEnumerator* pEnumerator);

    // VAD-based sentence splitter
    static void vadSentenceSplitter(const std::vector<BYTE>& newAudio, WAVEFORMATEX* pwfx, int& segmentIdx, const std::string& dateStr, bool forceFlush = false);
    static float frameRMS(const int16_t* samples, size_t count);
};