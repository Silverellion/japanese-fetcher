#include "audio_capturer.h"

#include <string>
#include <thread>
#include <windows.h>

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <regex>
#include <filesystem>
#include <deque>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "avrt.lib")

#define SEGMENTED_AUDIO_DIRECTORY std::string("C:\\japanese-fetcher\\Cache\\Audios\\")
#define FULL_AUDIO_DIRECTORY std::string("C:\\japanese-fetcher\\Saved\\Audios\\")

std::atomic<bool> AudioCapturer::recording{ false };
std::thread AudioCapturer::captureThread;
std::mutex AudioCapturer::recordMutex;
std::vector<BYTE> AudioCapturer::fullRecordingData;

void AudioCapturer::startAudioCapture(int secondsPerFile) {
    std::lock_guard<std::mutex> lock(recordMutex);
    if (recording) return;
    recording = true;
    fullRecordingData.clear();
    captureThread = std::thread(captureLoop, secondsPerFile);
}

void AudioCapturer::stopAudioCapture() {
    std::lock_guard<std::mutex> lock(recordMutex);
    if (!recording) return;
    recording = false;
    if (captureThread.joinable()) {
        captureThread.join();
    }
}

bool AudioCapturer::isRecording() {
    return recording;
}

void AudioCapturer::captureLoop(int secondsPerFile) {
    CoInitialize(nullptr);

    IMMDeviceEnumerator* pEnumerator = nullptr;
    IMMDevice* pDevice = nullptr;
    IAudioClient* pAudioClient = nullptr;
    IAudioCaptureClient* pCaptureClient = nullptr;

    if (!initializeAudioDevices(&pEnumerator, &pDevice, &pAudioClient, &pCaptureClient)) {
        std::cerr << "Failed to initialize audio devices" << std::endl;
        return;
    }

    WAVEFORMATEX* pwfx = nullptr;
    pAudioClient->GetMixFormat(&pwfx);

    const int bytesPerSec = pwfx->nAvgBytesPerSec;
    std::vector<BYTE> audioData;
    audioData.reserve(bytesPerSec * 4);

    int segmentIdx = 1;
    std::string dateStr = getCurrentDateString();

    auto segmentStartTime = std::chrono::steady_clock::now();
    bool inSilence = false;
    int silenceMs = 0;

    std::deque<float> rmsHistory;
    constexpr int RMS_WINDOW_SIZE = 10;

    while (recording) {
        std::vector<BYTE> capturedBuffer;
        processAudioBuffer(pCaptureClient, pwfx->nBlockAlign, capturedBuffer);

        if (!capturedBuffer.empty()) {
            fullRecordingData.insert(fullRecordingData.end(), capturedBuffer.begin(), capturedBuffer.end());
            audioData.insert(audioData.end(), capturedBuffer.begin(), capturedBuffer.end());
        }

        // Calculate RMS for the last 80ms and keep history
        float rms = calculateRMS(audioData, pwfx->nChannels, 80);
        rmsHistory.push_back(rms);
        if (rmsHistory.size() > RMS_WINDOW_SIZE) rmsHistory.pop_front();

        bool sustainedSilence = false;
        if (rmsHistory.size() == RMS_WINDOW_SIZE) {
            int silentFrames = 0;
            for (float v : rmsHistory) {
                if (v < SILENCE_THRESHOLD * 2) silentFrames++;
            }
            if (silentFrames >= RMS_WINDOW_SIZE - 2) sustainedSilence = true;
        }

        auto now = std::chrono::steady_clock::now();
        auto segmentDuration = std::chrono::duration_cast<std::chrono::milliseconds>(now - segmentStartTime);

        if (audioData.size() > bytesPerSec * MIN_SEGMENT_DURATION_SEC &&
            (sustainedSilence || isGoodSplitPoint(audioData, pwfx->nSamplesPerSec, pwfx->nChannels))) {
            saveSegmentedAudioFile(audioData, pwfx, segmentIdx, dateStr);
            audioData.clear();
            segmentStartTime = now;
            segmentIdx++;
            rmsHistory.clear();
            continue;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (!audioData.empty()) {
        saveSegmentedAudioFile(audioData, pwfx, segmentIdx, dateStr);
    }

    if (!fullRecordingData.empty()) {
        saveFullAudioFile(fullRecordingData, pwfx, dateStr);
    }

    cleanupAudioDevices(pwfx, pCaptureClient, pAudioClient, pDevice, pEnumerator);
    CoUninitialize();
}

std::string AudioCapturer::getCurrentDateString() {
    int recordingNumber = 1;
    std::regex recordingPattern("RECORDING_([0-9]+)_");

    try {
        for (const auto& entry : std::filesystem::directory_iterator(SEGMENTED_AUDIO_DIRECTORY)) {
            if (entry.is_regular_file() && entry.path().extension() == ".wav") {
                std::string filename = entry.path().filename().string();
                std::smatch matches;
                if (std::regex_search(filename, matches, recordingPattern) && matches.size() > 1) {
                    int foundNumber = std::stoi(matches[1].str());
                    if (foundNumber >= recordingNumber) {
                        recordingNumber = foundNumber + 1;
                    }
                }
            }
        }

        for (const auto& entry : std::filesystem::directory_iterator(FULL_AUDIO_DIRECTORY)) {
            if (entry.is_regular_file() && entry.path().extension() == ".wav") {
                std::string filename = entry.path().filename().string();
                std::smatch matches;
                if (std::regex_search(filename, matches, recordingPattern) && matches.size() > 1) {
                    int foundNumber = std::stoi(matches[1].str());
                    if (foundNumber >= recordingNumber) {
                        recordingNumber = foundNumber + 1;
                    }
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error finding recording number: " << e.what() << std::endl;
    }

    std::time_t t = std::time(nullptr);
    std::tm tm{};
    localtime_s(&tm, &t);
    std::ostringstream oss;
    oss << "RECORDING_" << recordingNumber << "_"
        << std::setw(2) << std::setfill('0') << tm.tm_mday << "_"
        << std::setw(2) << std::setfill('0') << (tm.tm_mon + 1) << "_"
        << (tm.tm_year + 1900);

    return oss.str();
}

void AudioCapturer::writeWavHeader(std::ofstream& out, int sampleRate, int bitsPerSample, int channels, size_t dataSize) {
    int byteRate = sampleRate * channels * bitsPerSample / 8;        // Number of bytes per second of audio playback
    int blockAlign = channels * bitsPerSample / 8;                   // Size of one sample frame (includes all channels)

    out.write("RIFF", 4);                                            // Resource Interchange File Format - container format for multimedia

    size_t chunkSize = 36 + dataSize;                                // File size minus 8 bytes (RIFF header and this chunkSize field)
    // 36 = 4 ("WAVE") + 24 (format section) + 8 ("data" header)

    out.write(reinterpret_cast<const char*>(&chunkSize), 4);         // Total size of file content after this point

    out.write("WAVE", 4);                                            // Specifies the file format is WAVE (used for audio)
    out.write("fmt ", 4);                                            // "fmt " = "format" - marks the beginning of format information

    int subchunk1Size = 16;                                          // Format section size for PCM = 16 bytes (fixed for uncompressed audio)
    short audioFormat = 1;                                           // Format code: 1 = PCM (Pulse Code Modulation = raw audio, no compression)
    out.write(reinterpret_cast<const char*>(&subchunk1Size), 4);     // Number of bytes in the format chunk (16 for PCM)
    out.write(reinterpret_cast<const char*>(&audioFormat), 2);       // Audio format type (1 = PCM)
    out.write(reinterpret_cast<const char*>(&channels), 2);          // Number of audio channels: 1 = mono, 2 = stereo
    out.write(reinterpret_cast<const char*>(&sampleRate), 4);        // Sample rate in Hz: e.g., 44100 = CD quality
    out.write(reinterpret_cast<const char*>(&byteRate), 4);          // Bytes per second = sampleRate * channels * bitsPerSample / 8
    out.write(reinterpret_cast<const char*>(&blockAlign), 2);        // Block alignment = channels * bitsPerSample / 8
    out.write(reinterpret_cast<const char*>(&bitsPerSample), 2);     // Bits per single audio sample (usually 16 for standard PCM)

    out.write("data", 4);                                            // "data" marks the start of audio data section
    out.write(reinterpret_cast<const char*>(&dataSize), 4);          // Number of bytes of actual audio data (not counting the header)
}

bool AudioCapturer::initializeAudioDevices(IMMDeviceEnumerator** pEnumerator, IMMDevice** pDevice, IAudioClient** pAudioClient, IAudioCaptureClient** pCaptureClient) {
    // Create an instance of the audio device enumerator
    HRESULT hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator), (void**)pEnumerator);
    if (FAILED(hr)) return false;

    // Get the default output (render) audio device
    hr = (*pEnumerator)->GetDefaultAudioEndpoint(eRender, eConsole, pDevice);
    if (FAILED(hr)) return false;

    // Get the audio client interface
    hr = (*pDevice)->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)pAudioClient);
    if (FAILED(hr)) return false;

    // Get the default audio format used by the audio engine
    WAVEFORMATEX* pwfx = nullptr;
    (*pAudioClient)->GetMixFormat(&pwfx);

    // Initialize for shared mode, loopback capture (captures output)
    hr = (*pAudioClient)->Initialize(AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_LOOPBACK,
        10000000, 0, pwfx, nullptr);
    if (FAILED(hr)) return false;

    // Get interface to capture audio data
    hr = (*pAudioClient)->GetService(__uuidof(IAudioCaptureClient), (void**)pCaptureClient);
    if (FAILED(hr)) return false;

    // Start capturing audio
    hr = (*pAudioClient)->Start();
    if (FAILED(hr)) return false;

    return true;
}

void AudioCapturer::processAudioBuffer(IAudioCaptureClient* pCaptureClient, int blockAlign, std::vector<BYTE>& audioData) {
    UINT32 packetLength = 0;
    pCaptureClient->GetNextPacketSize(&packetLength);               // Check if there's any audio packet available

    while (packetLength != 0) {
        BYTE* pData;
        UINT32 numFramesAvailable;
        DWORD flags;
        // Get pointer to captured audio data
        pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, nullptr, nullptr);
        UINT32 bytesToCopy = numFramesAvailable * blockAlign;       // Calculate total size in bytes

        float* samples = (float*)pData;                             // Input format is 32-bit float
        size_t sampleCount = bytesToCopy / sizeof(float);
        std::vector<short> scaled(sampleCount);                     // Convert to 16-bit signed integer for WAV format
        // Clamp sample value to valid range
        for (size_t i = 0; i < sampleCount; ++i) {
            if (samples[i] > 1.0f) samples[i] = 1.0f;
            if (samples[i] < -1.0f) samples[i] = -1.0f;
            scaled[i] = static_cast<short>(std::round(samples[i] * 32767.0f)); // Scale from [-1.0, 1.0] float to [-32768, 32767] short
        }
        audioData.insert(audioData.end(), (BYTE*)scaled.data(), (BYTE*)scaled.data() + sampleCount * sizeof(short)); // Append converted audio

        pCaptureClient->ReleaseBuffer(numFramesAvailable);          // Release current packet
        pCaptureClient->GetNextPacketSize(&packetLength);           // Check for more packets
    }
}

void AudioCapturer::saveSegmentedAudioFile(const std::vector<BYTE>& audioData, WAVEFORMATEX* pwfx, int segmentIdx, const std::string& dateStr) {
    std::ostringstream oss;
    oss << SEGMENTED_AUDIO_DIRECTORY << dateStr << "_SEGMENT_" << segmentIdx << ".wav";
    std::string filename = oss.str();
    std::ofstream out(filename, std::ios::binary);
    writeWavHeader(out, pwfx->nSamplesPerSec, 16, pwfx->nChannels, audioData.size());
    out.write(reinterpret_cast<const char*>(audioData.data()), audioData.size());
    out.close();
}

void AudioCapturer::saveFullAudioFile(const std::vector<BYTE>& audioData, WAVEFORMATEX* pwfx, const std::string& dateStr) {
    std::ostringstream oss;
    oss << FULL_AUDIO_DIRECTORY << dateStr << ".wav";
    std::string filename = oss.str();
    std::ofstream out(filename, std::ios::binary);
    writeWavHeader(out, pwfx->nSamplesPerSec, 16, pwfx->nChannels, audioData.size());
    out.write(reinterpret_cast<const char*>(audioData.data()), audioData.size());
    out.close();
}

void AudioCapturer::cleanupAudioDevices(WAVEFORMATEX* pwfx, IAudioCaptureClient* pCaptureClient, IAudioClient* pAudioClient, IMMDevice* pDevice, IMMDeviceEnumerator* pEnumerator) {
    CoTaskMemFree(pwfx);
    pCaptureClient->Release();
    pAudioClient->Release();
    pDevice->Release();
    pEnumerator->Release();
}

float AudioCapturer::calculateRMS(const std::vector<BYTE>& audioData, int channels, int durationMs) {
    if (audioData.size() < sizeof(short)) return 0.0f;

    const short* samples = reinterpret_cast<const short*>(audioData.data());
    size_t sampleCount = audioData.size() / sizeof(short);

    // How many samples to use (based on time) Sample rate in Hz, 44100 = CD quality
    size_t samplesForDuration = static_cast<size_t>((44100 * channels * durationMs) / 1000);
    size_t samplesToCheck = (sampleCount < samplesForDuration) ? sampleCount : samplesForDuration;

    if (samplesToCheck == 0) return 0.0f;

    double sumSquares = 0.0;
    for (size_t i = sampleCount - samplesToCheck; i < sampleCount; ++i) {
        double normalized = static_cast<double>(samples[i]) / 32767.0; // Normalize sample to [-1.0, 1.0]
        sumSquares += normalized * normalized;
    }

    return static_cast<float>(std::sqrt(sumSquares / samplesToCheck));
}

float AudioCapturer::calculateZCR(const std::vector<BYTE>& audioData, int channels, int durationMs) {
    if (audioData.size() < sizeof(short) * 2) return 0.0f;
    const short* samples = reinterpret_cast<const short*>(audioData.data());
    size_t sampleCount = audioData.size() / sizeof(short);

    size_t samplesForDuration = static_cast<size_t>((44100 * channels * durationMs) / 1000);
    size_t start = sampleCount > samplesForDuration ? sampleCount - samplesForDuration : 0;
    size_t end = sampleCount;

    int zeroCrossings = 0;
    for (size_t i = start + 1; i < end; ++i) {
        if ((samples[i - 1] >= 0 && samples[i] < 0) || (samples[i - 1] < 0 && samples[i] >= 0)) {
            zeroCrossings++;
        }
    }
    float zcr = static_cast<float>(zeroCrossings) / (end - start);
    return zcr;
}

bool AudioCapturer::detectSilence(const std::vector<BYTE>& audioData, int sampleRate, int channels, int durationMs) {
    float rms = calculateRMS(audioData, channels, durationMs);
    constexpr float SILENCE_THRESHOLD = 0.005f;
    return rms < SILENCE_THRESHOLD;
}

bool AudioCapturer::isGoodSplitPoint(const std::vector<BYTE>& audioData, int sampleRate, int channels) {
    float rms = calculateRMS(audioData, channels, 80);
    float zcr = calculateZCR(audioData, channels, 80);

    if (rms < 0.008f && zcr > 0.08f) {
        return true;
    }

    if (detectSilence(audioData, sampleRate, channels, 120)) {
        return true;
    }

    float currentRMS = calculateRMS(audioData, channels, 120);
    float previousRMS = calculateRMS(audioData, channels, 240);

    if (previousRMS > 0.02f && currentRMS < previousRMS * 0.4f && currentRMS < 0.01f) {
        return true;
    }

    return false;
}