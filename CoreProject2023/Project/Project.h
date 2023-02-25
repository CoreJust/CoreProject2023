#pragma once
#include "ProjectSettings.h"

class Project final {
private:
	ProjectSettings m_settings;

public:
	Project(const std::string& projectFile);

	ProjectSettings& getSettings();
};

extern ProjectSettings* g_settings;