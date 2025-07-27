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

#include <vector>
#include <cmath>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "avrt.lib")

#define SEGMENTED_AUDIO_DIRECTORY std::string("C:\\live-furigana\\Cache\\Audios\\")
#define FULL_AUDIO_DIRECTORY std::string("C:\\live-furigana\\Saved\\Audios\\")

std::atomic<bool> AudioCapturer::recording{ false };
std::thread AudioCapturer::captureThread;
std::mutex AudioCapturer::recordMutex;
std::vector<BYTE> AudioCapturer::fullRecordingData;

namespace {
    std::vector<BYTE> vadBuffer;
    int silenceFrames = 0;
    int speechFrames = 0;
    bool inSpeech = false;
    std::vector<BYTE> leftoverBytes;
}

float findPeakAbs(const int16_t* samples, size_t count) {
    float peak = 0.0f;
    for (size_t i = 0; i < count; ++i) {
        float absVal = std::abs(samples[i] / 32768.0f);
        if (absVal > peak) peak = absVal;
    }
    return peak;
}

void applyGain(int16_t* samples, size_t count, float gain) {
    for (size_t i = 0; i < count; ++i) {
        float v = samples[i] * gain;
        if (v > 32767.0f) v = 32767.0f;
        if (v < -32768.0f) v = -32768.0f;
        samples[i] = static_cast<int16_t>(std::round(v));
    }
}

void AudioCapturer::startAudioCapture(int secondsPerFile) {
    std::lock_guard<std::mutex> lock(recordMutex);
    if (recording) return;
    recording = true;
    fullRecordingData.clear();
    leftoverBytes.clear();
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
    const int sampleRate = pwfx->nSamplesPerSec;
    const int channels = pwfx->nChannels;
    std::vector<BYTE> audioData;
    audioData.reserve(bytesPerSec * 4);

    int segmentIdx = 1;
    std::string dateStr = getCurrentDateString();

    vadBuffer.clear();
    silenceFrames = 0;
    speechFrames = 0;
    inSpeech = false;
    leftoverBytes.clear();

    while (recording) {
        std::vector<BYTE> capturedBuffer;
        processAudioBuffer(pCaptureClient, pwfx->nBlockAlign, capturedBuffer);

        if (!capturedBuffer.empty()) {
            leftoverBytes.insert(leftoverBytes.end(), capturedBuffer.begin(), capturedBuffer.end());
            vadSentenceSplitter({}, pwfx, segmentIdx, dateStr); 
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    vadSentenceSplitter({}, pwfx, segmentIdx, dateStr, true);

    if (!fullRecordingData.empty()) {
        int16_t* samples = reinterpret_cast<int16_t*>(fullRecordingData.data());
        size_t sampleCount = fullRecordingData.size() / sizeof(int16_t);
        float peak = findPeakAbs(samples, sampleCount);
        float target = 0.98f;
        float gain = (peak > 0.0001f && peak < target) ? (target / peak) : 1.0f;
        if (gain > 1.01f) applyGain(samples, sampleCount, gain);
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

            float v = samples[i] * VOLUME_MULTIPLIER;
            if (v > 1.0f) v = 1.0f;
            if (v < -1.0f) v = -1.0f;
            scaled[i] = static_cast<short>(std::round(v * 32767.0f));
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

float AudioCapturer::frameRMS(const int16_t* samples, size_t count) {
    double sum = 0.0;
    for (size_t i = 0; i < count; ++i) {
        double v = samples[i] / 32768.0;
        sum += v * v;
    }
    return static_cast<float>(sqrt(sum / count));
}

void AudioCapturer::vadSentenceSplitter(const std::vector<BYTE>&, WAVEFORMATEX* pwfx, int& segmentIdx, const std::string& dateStr, bool forceFlush) {
    const int sampleRate = pwfx->nSamplesPerSec;
    const int channels = pwfx->nChannels;
    const int bytesPerSample = 2;
    const int frameSamples = (sampleRate * VAD_FRAME_MS) / 1000;
    const int frameBytes = frameSamples * bytesPerSample * channels;
    size_t offset = 0;

    while (leftoverBytes.size() - offset >= frameBytes) {
        const int16_t* frame = reinterpret_cast<const int16_t*>(&leftoverBytes[offset]);
        float rms = frameRMS(frame, frameSamples * channels);

        vadBuffer.insert(vadBuffer.end(), (BYTE*)frame, (BYTE*)frame + frameBytes);

        if (rms > VAD_ENERGY_THRESHOLD) {
            speechFrames++;
            silenceFrames = 0;
            inSpeech = true;
        }
        else {
            if (inSpeech) silenceFrames++;
            if (inSpeech && silenceFrames >= VAD_MIN_SILENCE_FRAMES && speechFrames >= VAD_MIN_SPEECH_FRAMES) {
                if (!vadBuffer.empty()) {
                    int16_t* segSamples = reinterpret_cast<int16_t*>(vadBuffer.data());
                    size_t segCount = vadBuffer.size() / sizeof(int16_t);
                    float peak = findPeakAbs(segSamples, segCount);
                    float target = 0.98f;
                    float gain = (peak > 0.0001f && peak < target) ? (target / peak) : 1.0f;
                    if (gain > 1.01f) applyGain(segSamples, segCount, gain);
                }
                saveSegmentedAudioFile(vadBuffer, pwfx, segmentIdx++, dateStr);
                fullRecordingData.insert(fullRecordingData.end(), vadBuffer.begin(), vadBuffer.end());

                vadBuffer.clear();
                silenceFrames = 0;
                speechFrames = 0;
                inSpeech = false;
            }
        }
        offset += frameBytes;
    }
    if (offset > 0) {
        leftoverBytes.erase(leftoverBytes.begin(), leftoverBytes.begin() + offset);
    }

    if (forceFlush && !vadBuffer.empty()) {
        int16_t* segSamples = reinterpret_cast<int16_t*>(vadBuffer.data());
        size_t segCount = vadBuffer.size() / sizeof(int16_t);
        float peak = findPeakAbs(segSamples, segCount);
        float target = 0.98f;
        float gain = (peak > 0.0001f && peak < target) ? (target / peak) : 1.0f;
        if (gain > 1.01f) applyGain(segSamples, segCount, gain);

        saveSegmentedAudioFile(vadBuffer, pwfx, segmentIdx++, dateStr);
        fullRecordingData.insert(fullRecordingData.end(), vadBuffer.begin(), vadBuffer.end());

        vadBuffer.clear();
        silenceFrames = 0;
        speechFrames = 0;
        inSpeech = false;
    }
}