#include "utility.h"

#include <windows.h>
#include <filesystem>
#include <iostream>

#define DEFAULT_DIRECTORY std::string("C:\\live-furigana\\")

void Utility::initializeDirectory() {
    if (!std::filesystem::exists(DEFAULT_DIRECTORY)) {
         std::filesystem::create_directory(DEFAULT_DIRECTORY);
    }

    const std::vector<std::pair<std::string, std::vector<std::string>>> directoryGroups = {
        {"Saved\\", {"Audios", "Transcripts", "Models"}},
        {"Cache\\", {"Audios", "Transcripts"}}
    };

    for (const auto& [base, subdirs] : directoryGroups) {
        std::string baseDir = DEFAULT_DIRECTORY + base;
        if (!std::filesystem::exists(baseDir)) {
             std::filesystem::create_directory(baseDir);
        }

        for (const auto& sub : subdirs) {
            std::filesystem::path path = baseDir + sub;
            if (!std::filesystem::exists(path)) {
                 std::filesystem::create_directory(path);
            }
        }
    }
}


std::string Utility::getExecutableDir() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::filesystem::path exePath(buffer);
    return exePath.parent_path().string() + "\\";
}

bool Utility::checkDirectory(std::string fullPath) {
	return (std::filesystem::exists(fullPath) && std::filesystem::is_directory(fullPath));
}