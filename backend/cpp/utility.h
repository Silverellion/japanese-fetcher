#pragma once
#include <string>

class Utility {
public:
	static void initializeDirectory();

private:
	bool checkDirectory(std::string fullPath);
};