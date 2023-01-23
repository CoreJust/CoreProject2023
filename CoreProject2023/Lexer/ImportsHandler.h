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

	std::vector<std::string> getImportedFiles();

private:
	void handleSingleImport(const std::string& importPath);
	void handleAllImport(const std::string& importPath);

	std::string findFullImportPath(const std::string& module, const std::vector<std::string>& currImportPaths);

	static const std::vector<std::string>& getCurrFileImportPaths();
};