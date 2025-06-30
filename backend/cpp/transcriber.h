#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <set>

class Transcriber {
public:
    static void startTranscription();
    static void stopTranscription();

private:
    static void monitorAudioDirectory();
    static void transcribeFile(const std::string& audioFilePath);
    static std::string getOutputBaseName(const std::string& audioFilePath);

    static std::atomic<bool> running;
    static std::thread monitorThread;
    static std::set<std::string> processedFiles;
};