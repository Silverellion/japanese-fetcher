#include "utility.h"
#include "audio_capturer.h"

int main() {
	Utility::initializeDirectory();
	AudioCapturer::startAudioCapture();
}