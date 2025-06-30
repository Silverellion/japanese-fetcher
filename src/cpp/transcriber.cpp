#include "transcriber.h"

#include <filesystem>
#include <iostream>
#include <chrono>

#define AUDIO_DIRECTORY std::string("C:\\japanese-fetcher\\audios\\")
#define TRANSCRIPT_DIRECTORY std::string("C:\\japanese-fetcher\\transcripts\\")
#define MODEL_PATH std::string("C:\\japanese-fetcher\\models\\ggml-medium.bin")
#define WHISPER_EXE std::string("external\\whisper.cpp\\whisper-cli.exe")

std::atomic<bool> Transcriber::running{ false };
std::thread Transcriber::monitorThread;
std::set<std::string> Transcriber::processedFiles;

void Transcriber::startTranscription() {
    if (running) return;

    running = true;
    monitorThread = std::thread(monitorAudioDirectory);
    std::cout << "Transcription monitoring started." << std::endl;
}

void Transcriber::stopTranscription() {
    if (!running) return;

    running = false;
    if (monitorThread.joinable()) {
        monitorThread.join();
    }
    std::cout << "Transcription monitoring stopped." << std::endl;
}

void Transcriber::monitorAudioDirectory() {
    while (running) {
        try {
            for (const auto& entry : std::filesystem::directory_iterator(AUDIO_DIRECTORY)) {
                if (entry.is_regular_file() && entry.path().extension() == ".wav") {
                    std::string filePath = entry.path().string();

                    if (processedFiles.find(filePath) == processedFiles.end()) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));

                        transcribeFile(filePath);
                        processedFiles.insert(filePath);
                    }
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error monitoring directory: " << e.what() << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

void Transcriber::transcribeFile(const std::string& audioFilePath) {
    std::string outputBase = getOutputBaseName(audioFilePath);

    std::string command = WHISPER_EXE +
        " -m " + MODEL_PATH +
        " -f \"" + audioFilePath + "\"" +
        " -of \"" + outputBase + "\"" +
        " --output-txt --output-srt";

    int result = system(command.c_str());
    if (result == 0) {
        std::cout << "Transcribed: " << std::filesystem::path(audioFilePath).filename().string() << std::endl;
    }
    else {
        std::cerr << "Failed to transcribe: " << std::filesystem::path(audioFilePath).filename().string() << std::endl;
    }
}

std::string Transcriber::getOutputBaseName(const std::string& audioFilePath) {
    std::filesystem::path path(audioFilePath);
    std::string filename = path.stem().string();
    return TRANSCRIPT_DIRECTORY + filename;
}