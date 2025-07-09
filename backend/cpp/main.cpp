#include "utility.h"
#include "audio_capturer.h"
#include "transcriber.h"
#include <string>
#include <iostream>

void printUsage() {
    std::cout << "Usage: japanese-fetcher-backend [options]\n"
              << "Options:\n"
              << "  --start-recording   Start in recording mode\n"
              << "  --command <cmd>     Execute command (start-recording, stop-recording, get-status, exit)\n";
}

int main(int argc, char* argv[]) {
    Utility::initializeDirectory();
    Transcriber::startTranscription();
    
    bool shouldRecord = false;
    std::string command;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--start-recording") {
            shouldRecord = true;
        }
        else if (arg == "--command" && i + 1 < argc) {
            command = argv[++i];
        }
        else if (arg == "--help") {
            printUsage();
            return 0;
        }
    }
    
    if (!command.empty()) {
        if (command == "start-recording") {
            AudioCapturer::startAudioCapture();
            std::cout << "Recording started" << std::endl;
            return 0;
        }
        else if (command == "stop-recording") {
            AudioCapturer::stopAudioCapture();
            std::cout << "Recording stopped" << std::endl;
            return 0;
        }
        else if (command == "get-status") {
            std::cout << (AudioCapturer::isRecording() ? "recording" : "not-recording") << std::endl;
            return 0;
        }
        else if (command == "exit") {
            AudioCapturer::stopAudioCapture();
            Transcriber::stopTranscription();
            std::cout << "Exiting" << std::endl;
            return 0;
        }
    }
    
    if (shouldRecord) {
        AudioCapturer::startAudioCapture();
    }
    std::cout << "Backend running. Press Ctrl+C to exit." << std::endl;
    
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}