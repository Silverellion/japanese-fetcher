#include "transcriber.h"
#include "utility.h"

#include <windows.h>
#include <filesystem>
#include <iostream>
#include <chrono>

#define AUDIO_DIRECTORY std::string("C:\\japanese-fetcher\\audios\\")
#define TRANSCRIPT_DIRECTORY std::string("C:\\japanese-fetcher\\transcripts\\")

std::string exeDir = Utility::getExecutableDir();
std::string whisperExe = exeDir + "external\\whisper.cpp\\whisper-cli.exe";
std::string modelPath = "C:\\japanese-fetcher\\models\\ggml-medium.bin";

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

bool Transcriber::runProcessWithWorkingDir(const std::string& command, const std::string& workingDir) {
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    char cmd[MAX_PATH * 4];
    strncpy_s(cmd, command.c_str(), sizeof(cmd) - 1);

    BOOL success = CreateProcessA(
        NULL,                   // use command line to find exe
        cmd,                    // command line to run
        NULL,                   // default process security
        NULL,                   // default thread security
        FALSE,                  // don't inherit handles
        CREATE_NO_WINDOW,       // run without showing a window
        NULL,                   // use parent's environment
        workingDir.c_str(),     // set working directory
        &si,                    // startup info
        &pi                     // receives process info (PID)
    );

    if (!success) {
        std::cerr << "Failed to launch process. Error: " << GetLastError() << std::endl;
        return false;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return exitCode == 0;
}

void Transcriber::transcribeFile(const std::string& audioFilePath) {
    std::string outputBase = getOutputBaseName(audioFilePath);

    std::string command = "\"" + whisperExe + "\"" +
        " -m \"" + modelPath + "\"" +
        " -f \"" + audioFilePath + "\"" +
        " -of \"" + outputBase + "\"" +
        " --language ja" + // TODO: Add language selector
        " --output-txt --output-srt";

    std::string whisperDir = exeDir + "external\\whisper.cpp\\";

    std::cout << "Running: " << command << "\nWorking dir: " << whisperDir << std::endl;

    bool ok = runProcessWithWorkingDir(command, whisperDir);
    if (ok) {
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