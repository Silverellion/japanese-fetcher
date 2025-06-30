#include "utility.h"
#include "audio_capturer.h"
#include "transcriber.h"

int main() {
    Utility::initializeDirectory();
    Transcriber::startTranscription();
    AudioCapturer::startAudioCapture();
    Transcriber::stopTranscription();

    return 0;
}