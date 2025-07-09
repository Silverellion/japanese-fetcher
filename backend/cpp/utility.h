#pragma once
#include <string>

class Utility {
public:
	static void initializeDirectory();
	static std::string getExecutableDir();

private:
	bool checkDirectory(std::string fullPath);
};