#pragma once

#include <mmdeviceapi.h>
#include <audioclient.h>

#include <vector>

#include <fstream>
#include <iostream>

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
private:
	static void writeWavHeader(std::ofstream& out, int sampleRate, int bitsPerSample, int channels, size_t dataSize);
	static bool initializeAudioDevices(IMMDeviceEnumerator** pEnumerator,
                                       IMMDevice** pDevice,
                                       IAudioClient** pAudioClient, 
                                       IAudioCaptureClient** pCaptureClient);
	static void processAudioBuffer(IAudioCaptureClient* pCaptureClient, int blockAlign, std::vector<BYTE>& audioData);
	static void saveAudioFile(const std::vector<BYTE>& audioData, WAVEFORMATEX* pwfx, int fileCount);
	static void cleanupAudioDevices(WAVEFORMATEX* pwfx,
                                    IAudioCaptureClient* pCaptureClient, 
                                    IAudioClient* pAudioClient, 
                                    IMMDevice* pDevice, 
                                    IMMDeviceEnumerator* pEnumerator);
};
