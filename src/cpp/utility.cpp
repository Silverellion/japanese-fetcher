#include "utility.h"

#include <filesystem>

#define DEFAULT_DIRECTORY "C:\\japanese-fetcher\\"

void initializeDirectory(std::string directoryName) {
	std::string fullPath = DEFAULT_DIRECTORY + directoryName;
	if (!(std::filesystem::exists(fullPath) && std::filesystem::is_directory(fullPath))) {
		std::filesystem::create_directory(fullPath);
	}
}