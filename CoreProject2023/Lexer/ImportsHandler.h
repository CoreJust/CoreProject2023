#pragma once
#include <vector>
#include <string>
#include <Utils/Defs.h>

extern std::vector<std::string> g_importPaths;

/*
	An auxilary class, used in Lexer.
	Converts a list of import ... to a list of certain files.
*/

class ImportsHandler final {
	std::vector<std::string> m_result;

public:
	void addImport(std::string module);

	// Get the resulting imported files list, clears stored list
	std::vector<std::string> getImportedFiles();

private:
	// Adding an import path(-s) to the list
	void handleSingleImport(const std::string& importPath);
	void handleAllImport(const std::string& importPath);

	// Searching for the path of the -module-'s import
	std::string findFullImportPath(const std::string& module, const std::vector<std::string>& currImportPaths);

	// Slicing current file's directory into { current, ../, ../../, etc }
	static const std::vector<std::string>& getCurrFileImportPaths();
};