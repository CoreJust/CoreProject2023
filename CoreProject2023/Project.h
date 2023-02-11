#pragma once
#include <string>
#include <vector>

class Project final {
private:
	

public:
	Project(const std::string& projectFile);
	Project(); // temporary

	std::string getMainFilePath();
	std::vector<std::string> getImportedPaths();

	std::string getAdditionalLinkDirectoriesString() const;
};