#include "utility.h"

#include <filesystem>

#define DEFAULT_DIRECTORY std::string("C:\\japanese-fetcher\\")

void Utility::initializeDirectory() {
    static const std::vector<std::string> subdirs = {
        "audios", "models", "transcripts", "test"
    };

    for (const auto& dir : subdirs) {
        std::filesystem::path path = DEFAULT_DIRECTORY + dir;
        if (!std::filesystem::is_directory(path)) {
            std::filesystem::create_directory(path);
        }
    }
}

bool Utility::checkDirectory(std::string fullPath) {
	return (std::filesystem::exists(fullPath) && std::filesystem::is_directory(fullPath));
}