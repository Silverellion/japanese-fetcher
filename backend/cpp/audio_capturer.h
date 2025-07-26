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
    static constexpr float SILENCE_THRESHOLD = 0.004f;
    static constexpr int SHORT_SILENCE_MS = 300;        // Short pause detection
    static constexpr int LONG_SILENCE_MS = 800;         // Long pause detection
    static constexpr int MIN_SEGMENT_DURATION_SEC = 1;  // Self-explanatory
    static constexpr int MAX_SEGMENT_DURATION_SEC = 4;  // Self-explanatory

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

    // === VAD functions (Voice Activity Detection) ===
    /**
    * @brief Calculates the Root Mean Square (RMS) value of the audio signal.
    *
    * RMS (Root Mean Square) is a measure of the signal's power, often used to estimate
    * perceived loudness in audio processing.
    *
    * Formula: RMS = sqrt((1/N) * sum(samples[i]^2))
    * - N is the number of samples
    * - samples[i] are 16-bit signed PCM values
    *
    * @param audioData   Raw audio data in 16-bit signed integer format.
    * @param channels    Number of audio channels (e.g., 1 for mono, 2 for stereo).
    * @param durationMs  Duration (in milliseconds) over which to calculate RMS. Default is 200 ms.
    * @return            RMS value as a float between 0.0 and 1.0.
    */
    static float calculateRMS(const std::vector<BYTE>& audioData, int channels, int durationMs = 80);
    static float calculateZCR(const std::vector<BYTE>& audioData, int channels, int durationMs = 80);
    static bool detectSilence(const std::vector<BYTE>& audioData, int sampleRate, int channels, int durationMs = 120);
    static bool isGoodSplitPoint(const std::vector<BYTE>& audioData, int sampleRate, int channels);
};