#include "Project.h"
#include "ProjectFileParser.h"
#include <Utils/ErrorManager.h>

ProjectSettings* g_settings;

Project::Project(const std::string& projectFile) {
	ErrorManager::init(ErrorManager::CONSOLE);
	g_settings = &m_settings;
	ProjectFileParser(m_settings).loadSettingsFromFile(projectFile);
}

ProjectSettings& Project::getSettings() {
	return m_settings;
}
