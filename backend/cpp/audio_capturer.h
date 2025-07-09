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
    * and saves it to WAV files of specified duration inside "C:\\japanese-fetcher\\audios".
    */
    static void startAudioCapture(int secondsPerFile = 1);
    static void stopAudioCapture();
    static bool isRecording();

private:
    static constexpr float SILENCE_THRESHOLD = 0.005f;  // Lower threshold
    static constexpr int SHORT_SILENCE_MS = 300;        // Short pause detection
    static constexpr int LONG_SILENCE_MS = 800;         // Long pause detection
    static constexpr int MIN_SEGMENT_DURATION_SEC = 1;  // Shorter minimum
    static constexpr int MAX_SEGMENT_DURATION_SEC = 4;  // Shorter maximum for "live" feel

    static std::atomic<bool> recording;
    static std::thread captureThread;
    static std::mutex recordMutex;
    
    static void captureLoop(int secondsPerFile);
    static std::string getCurrentDateString();
    static void writeWavHeader(std::ofstream& out, int sampleRate, int bitsPerSample, int channels, size_t dataSize);
    static bool initializeAudioDevices(IMMDeviceEnumerator** pEnumerator,
                                       IMMDevice** pDevice,
                                       IAudioClient** pAudioClient, 
                                       IAudioCaptureClient** pCaptureClient);
    static void processAudioBuffer(IAudioCaptureClient* pCaptureClient, int blockAlign, std::vector<BYTE>& audioData);
    static void saveAudioFile(const std::vector<BYTE>& audioData, WAVEFORMATEX* pwfx, int segmentIdx, const std::string& dateStr);
    static void cleanupAudioDevices(WAVEFORMATEX* pwfx,
                                    IAudioCaptureClient* pCaptureClient, 
                                    IAudioClient* pAudioClient, 
                                    IMMDevice* pDevice, 
                                    IMMDeviceEnumerator* pEnumerator);

    // VAD functions
    static float calculateRMS(const std::vector<BYTE>& audioData, int channels, int durationMs = 200);
    static bool detectSilence(const std::vector<BYTE>& audioData, int sampleRate, int channels, int durationMs = 200);
    static bool isGoodSplitPoint(const std::vector<BYTE>& audioData, int sampleRate, int channels);
    static void saveSegmentWithOverlap(std::vector<BYTE>& audioData, std::vector<BYTE>& overlapBuffer, 
                                       WAVEFORMATEX* pwfx, int& fileCount, int sampleRate, const std::string& dateStr);
};