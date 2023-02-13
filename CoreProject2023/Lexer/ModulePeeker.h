#pragma once
#include <vector>
#include <fstream>
#include <Utils/ErrorManager.h>
#include <Module/Symbols/Annotations.h>
#include <Module/Module.h>

// Looks for module's qualities and imports
class ModulePeeker final {
	ModuleQualities m_qualities;
	std::vector<std::string> m_imports;

	std::ifstream m_file;
	std::string m_modulePath;

	std::string m_buffer;
	std::string m_text;
	u64 m_pos = 0;
	u64 m_line = 1;
	u64 m_nextLine = 1;

public:
	ModulePeeker(std::string modulePath);

	ModuleRef load();

private:
	void processModuleQualities();
	void processImports();

private:
	void skipComment();
	void skipString();

	// reads a word and stores it to to
	void loadIdentifier(std::string& to);

private:
	void skipWhitespaces(bool spacesOnly);

	char next();
};