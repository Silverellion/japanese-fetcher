#include "utility.h"
#include "audio_capturer.h"

int main() {
	initializeDirectory("audios");
	initializeDirectory("models");
	initializeDirectory("transcripts");

	startAudioCapture("audios");
}