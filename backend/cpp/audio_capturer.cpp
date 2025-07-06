#include "audio_capturer.h"

#include <string>
#include <thread>
#include <windows.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "avrt.lib")

#define AUDIO_DIRECTORY std::string("C:\\japanese-fetcher\\audios\\")

void AudioCapturer::writeWavHeader(std::ofstream& out, int sampleRate, int bitsPerSample, int channels, size_t dataSize) {
    int byteRate = sampleRate * channels * bitsPerSample / 8;
    int blockAlign = channels * bitsPerSample / 8;
    out.write("RIFF", 4);
    size_t chunkSize = 36 + dataSize;
    out.write(reinterpret_cast<const char*>(&chunkSize), 4);
    out.write("WAVE", 4);

    // fmt chunk
    out.write("fmt ", 4);
    int subchunk1Size = 16;
    short audioFormat = 1; // PCM
    out.write(reinterpret_cast<const char*>(&subchunk1Size), 4);
    out.write(reinterpret_cast<const char*>(&audioFormat), 2);
    out.write(reinterpret_cast<const char*>(&channels), 2);
    out.write(reinterpret_cast<const char*>(&sampleRate), 4);
    out.write(reinterpret_cast<const char*>(&byteRate), 4);
    out.write(reinterpret_cast<const char*>(&blockAlign), 2);
    out.write(reinterpret_cast<const char*>(&bitsPerSample), 2);

    // data chunk
    out.write("data", 4);
    out.write(reinterpret_cast<const char*>(&dataSize), 4);
}

bool AudioCapturer::initializeAudioDevices(IMMDeviceEnumerator** pEnumerator, IMMDevice** pDevice, IAudioClient** pAudioClient, IAudioCaptureClient** pCaptureClient) {
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator), (void**)pEnumerator);
    if (FAILED(hr)) return false;

    hr = (*pEnumerator)->GetDefaultAudioEndpoint(eRender, eConsole, pDevice);
    if (FAILED(hr)) return false;

    hr = (*pDevice)->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)pAudioClient);
    if (FAILED(hr)) return false;

    WAVEFORMATEX* pwfx = nullptr;
    (*pAudioClient)->GetMixFormat(&pwfx);

    hr = (*pAudioClient)->Initialize(AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_LOOPBACK,
        10000000, 0, pwfx, nullptr);
    if (FAILED(hr)) return false;

    hr = (*pAudioClient)->GetService(__uuidof(IAudioCaptureClient), (void**)pCaptureClient);
    if (FAILED(hr)) return false;

    hr = (*pAudioClient)->Start();
    if (FAILED(hr)) return false;

    return true;
}

void AudioCapturer::processAudioBuffer(IAudioCaptureClient* pCaptureClient, int blockAlign, std::vector<BYTE>& audioData) {
    UINT32 packetLength = 0;
    pCaptureClient->GetNextPacketSize(&packetLength);

    while (packetLength != 0) {
        BYTE* pData;
        UINT32 numFramesAvailable;
        DWORD flags;
        pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, nullptr, nullptr);
        UINT32 bytesToCopy = numFramesAvailable * blockAlign;

        float* samples = (float*)pData;
        size_t sampleCount = bytesToCopy / sizeof(float);
        std::vector<short> scaled(sampleCount);
        for (size_t i = 0; i < sampleCount; ++i) {
            if (samples[i] > 1.0f) samples[i] = 1.0f;
            if (samples[i] < -1.0f) samples[i] = -1.0f;
            scaled[i] = static_cast<short>(std::round(samples[i] * 32767.0f));
        }
        audioData.insert(audioData.end(), (BYTE*)scaled.data(), (BYTE*)scaled.data() + sampleCount * sizeof(short));

        pCaptureClient->ReleaseBuffer(numFramesAvailable);
        pCaptureClient->GetNextPacketSize(&packetLength);
    }
}

void AudioCapturer::saveAudioFile(const std::vector<BYTE>& audioData, WAVEFORMATEX* pwfx, int fileCount) {
    std::string filename = AUDIO_DIRECTORY + "capture_" + std::to_string(fileCount) + ".wav";
    std::ofstream out(filename, std::ios::binary);
    writeWavHeader(out, pwfx->nSamplesPerSec, 16, pwfx->nChannels, audioData.size());
    out.write(reinterpret_cast<const char*>(audioData.data()), audioData.size());
    out.close();
    std::cout << "Saved " << filename << " (" << audioData.size() / 1024 << " KB)" << std::endl;
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

    size_t samplesForDuration = static_cast<size_t>((44100 * channels * durationMs) / 1000);
    size_t samplesToCheck = (sampleCount < samplesForDuration) ? sampleCount : samplesForDuration;

    if (samplesToCheck == 0) return 0.0f;

    double sumSquares = 0.0;
    for (size_t i = sampleCount - samplesToCheck; i < sampleCount; ++i) {
        double normalized = static_cast<double>(samples[i]) / 32767.0;
        sumSquares += normalized * normalized;
    }

    return static_cast<float>(std::sqrt(sumSquares / samplesToCheck));
}

bool AudioCapturer::detectSilence(const std::vector<BYTE>& audioData, int sampleRate, int channels, int durationMs) {
    float rms = calculateRMS(audioData, channels, durationMs);
    return rms < SILENCE_THRESHOLD;
}

bool AudioCapturer::isGoodSplitPoint(const std::vector<BYTE>& audioData, int sampleRate, int channels) {
    if (detectSilence(audioData, sampleRate, channels, 200)) {
        return true;
    }

    float currentRMS = calculateRMS(audioData, channels, 100);
    float previousRMS = calculateRMS(audioData, channels, 200);

    return (currentRMS < previousRMS * 0.7f) && (currentRMS < SILENCE_THRESHOLD * 2.0f);
}

void AudioCapturer::saveSegmentWithOverlap(std::vector<BYTE>& audioData, std::vector<BYTE>& overlapBuffer,
    WAVEFORMATEX* pwfx, int& fileCount, int sampleRate) {
    if (audioData.empty()) return;

    size_t overlapSize = static_cast<size_t>(sampleRate * pwfx->nChannels * sizeof(short) * 0.5f);

    std::vector<BYTE> segmentData;
    segmentData.insert(segmentData.end(), overlapBuffer.begin(), overlapBuffer.end());
    segmentData.insert(segmentData.end(), audioData.begin(), audioData.end());

    saveAudioFile(segmentData, pwfx, fileCount++);

    overlapBuffer.clear();
    if (audioData.size() >= overlapSize) {
        overlapBuffer.insert(overlapBuffer.end(),
            audioData.end() - overlapSize, audioData.end());
    }
    else {
        overlapBuffer = audioData;
    }

    audioData.clear();
}

void AudioCapturer::startAudioCapture(int secondsPerFile) {
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
    const int minBufferSize = bytesPerSec * MIN_SEGMENT_DURATION_SEC;
    const int maxBufferSize = bytesPerSec * MAX_SEGMENT_DURATION_SEC;

    std::vector<BYTE> audioData;
    std::vector<BYTE> overlapBuffer;
    audioData.reserve(maxBufferSize);

    int fileCount = 0;
    auto segmentStartTime = std::chrono::steady_clock::now();
    auto lastCheck = std::chrono::steady_clock::now();

    std::cout << "Recording started with VAD (sample rate: " << pwfx->nSamplesPerSec
        << " Hz, channels: " << pwfx->nChannels << ")" << std::endl;
    std::cout << "Press Ctrl+C to stop." << std::endl;

    while (true) {
        processAudioBuffer(pCaptureClient, pwfx->nBlockAlign, audioData);

        auto now = std::chrono::steady_clock::now();
        auto segmentDuration = std::chrono::duration_cast<std::chrono::milliseconds>(now - segmentStartTime);
        auto checkInterval = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastCheck);

        if (checkInterval.count() >= 100 && audioData.size() >= minBufferSize) {

            if (segmentDuration.count() >= (MIN_SEGMENT_DURATION_SEC * 1000)) {
                if (isGoodSplitPoint(audioData, pwfx->nSamplesPerSec, pwfx->nChannels)) {
                    std::cout << "Silence detected, saving segment..." << std::endl;
                    saveSegmentWithOverlap(audioData, overlapBuffer, pwfx, fileCount, pwfx->nSamplesPerSec);
                    segmentStartTime = now;
                }
            }

            lastCheck = now;
        }

        if (segmentDuration.count() >= (MAX_SEGMENT_DURATION_SEC * 1000)) {
            std::cout << "Maximum segment duration reached, saving..." << std::endl;
            saveSegmentWithOverlap(audioData, overlapBuffer, pwfx, fileCount, pwfx->nSamplesPerSec);
            segmentStartTime = now;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    cleanupAudioDevices(pwfx, pCaptureClient, pAudioClient, pDevice, pEnumerator);
    CoUninitialize();
}