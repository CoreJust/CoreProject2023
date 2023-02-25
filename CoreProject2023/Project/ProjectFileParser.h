#pragma once
#include "ProjectSettings.h"

// Project settings are described in a JSON file
class ProjectFileParser final {
private:
	ProjectSettings& m_settings;
	std::string m_file;

public:
	ProjectFileParser(ProjectSettings& settings);

	void loadSettingsFromFile(const std::string& fileName);

private:
	void loadSettings(void* jsonData);

	std::string getAbsolutePath(const std::string& pathFromProjectFile);
};