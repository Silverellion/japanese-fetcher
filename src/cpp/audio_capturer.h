#pragma once
#include <string>

/**
* @brief Captures audio from the default playback device (loopback capture)
* @param secondsPerFile Duration of each captured audio file in seconds
* 
* This class records audio being played through the default audio output device
* and saves it to WAV files of specified duration inside "C:\\japanese-fetcher\\audios".
*/
void startAudioCapture(int secondsPerFile = 1);