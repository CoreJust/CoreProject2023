#pragma once
#include <string>
#include <vector>

class Project final {
private:
	

public:
	Project(const std::string& projectFile);
	Project(); // temporary

	std::string getMainFilePath();
};