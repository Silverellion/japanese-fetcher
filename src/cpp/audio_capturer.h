#pragma once
#include <string>

/**
* @brief Captures audio from the default playback device (loopback capture)
* @param outputDirectory Directory where captured audio files will be saved
* @param secondsPerFile Duration of each captured audio file in seconds
* 
* This class records audio being played through the default audio output device
* and saves it to WAV files of specified duration.
*/
void startAudioCapture(std::string outputDirectory, int secondsPerFile = 1);