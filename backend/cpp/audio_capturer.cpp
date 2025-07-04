#include "audio_capturer.h"

#include <string>
#include <windows.h>
#include <thread>

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

    hr = (*pEnumerator)->GetDefaultAudioEndpoint(eRender, eConsole, pDevice); // loopback capture
    if (FAILED(hr)) return false;

    hr = (*pDevice)->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)pAudioClient);
    if (FAILED(hr)) return false;

    WAVEFORMATEX* pwfx = nullptr;
    (*pAudioClient)->GetMixFormat(&pwfx); // Use shared format

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
            if (samples[i] >  1.0f) samples[i] =  1.0f;
            if (samples[i] < -1.0f) samples[i] = -1.0f;
            scaled[i] = static_cast<short>(std::roundf(samples[i] * 32767.0f));
        }
        audioData.insert(audioData.end(), (BYTE*)scaled.data(), (BYTE*)scaled.data() + sampleCount * sizeof(short));

        pCaptureClient->ReleaseBuffer(numFramesAvailable);
        pCaptureClient->GetNextPacketSize(&packetLength);
    }
}

void AudioCapturer::saveAudioFile(const std::vector<BYTE>& audioData, WAVEFORMATEX* pwfx, int fileCount) {
    std::string filename = AUDIO_DIRECTORY + "capture_" + std::to_string(fileCount) + ".wav";
    std::ofstream out(filename, std::ios::binary);
    writeWavHeader(out, pwfx->nSamplesPerSec, 16, pwfx->nChannels, audioData.size()); // Always write as 16-bit PCM
    out.write(reinterpret_cast<const char*>(audioData.data()), audioData.size());
    out.close();
    std::cout << "Saved " << filename << "\n";
}

void AudioCapturer::cleanupAudioDevices(WAVEFORMATEX* pwfx, IAudioCaptureClient* pCaptureClient, IAudioClient* pAudioClient, IMMDevice* pDevice, IMMDeviceEnumerator* pEnumerator) {
    CoTaskMemFree(pwfx);
    pCaptureClient->Release();
    pAudioClient->Release();
    pDevice->Release();
    pEnumerator->Release();
}

void AudioCapturer::startAudioCapture(int secondsPerFile) {
    CoInitialize(nullptr);

    IMMDeviceEnumerator* pEnumerator = nullptr;
    IMMDevice* pDevice = nullptr;
    IAudioClient* pAudioClient = nullptr;
    IAudioCaptureClient* pCaptureClient = nullptr;

    if (!initializeAudioDevices(&pEnumerator, &pDevice, &pAudioClient, &pCaptureClient)) return;

    WAVEFORMATEX* pwfx = nullptr;
    pAudioClient->GetMixFormat(&pwfx); // Use shared format

    const int blockAlign = pwfx->nBlockAlign;
    const int bytesPerSec = pwfx->nAvgBytesPerSec;
    const int bufferSize = bytesPerSec * secondsPerFile;

    std::vector<BYTE> audioData;
    audioData.reserve(bufferSize);

    int fileCount = 0;
    std::cout << "Recording started. Press Ctrl+C to stop." << std::endl;

    while (true) {
        processAudioBuffer(pCaptureClient, blockAlign, audioData);

        if (audioData.size() >= bufferSize) {
            saveAudioFile(audioData, pwfx, fileCount++);
            audioData.clear();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    cleanupAudioDevices(pwfx, pCaptureClient, pAudioClient, pDevice, pEnumerator);
    CoUninitialize();
}