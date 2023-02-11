#include "ImportsHandler.h"
#include <filesystem>
#include <Utils/ErrorManager.h>
#include <Utils/String.h>

std::vector<std::string> g_importPaths;

void ImportsHandler::addImport(std::string module) {
	// Getting file name
	size_t lastDot = module.find_last_of('.');
	std::string fileName = module.substr(lastDot + 1);

	// Converting path to usual view from core's format
	replaceChar(module, '.', '/');
	if (fileName == "*") {
		module.erase(module.size() - 1);
	} else {
		module.append(".core");
	}

	// Adding import
	const std::vector<std::string>& currImportPaths = getCurrFileImportPaths();
	std::string importPath = findFullImportPath(module, currImportPaths);
	if (importPath != "") {
		if (fileName == "*") {
			handleAllImport(importPath);
		} else {
			handleSingleImport(importPath);
		}
	}
}

std::vector<std::string> ImportsHandler::getImportedFiles() {
	return std::move(m_result);
}

void ImportsHandler::handleSingleImport(const std::string& importPath) {
	m_result.push_back(importPath);
}

void ImportsHandler::handleAllImport(const std::string& importPath) {
	for (const auto& f : std::filesystem::directory_iterator(importPath)) {
		if (f.path().has_extension()
			&& f.path().extension() == ".core"
			&& f.path().string() != g_currFilePath) {
			m_result.push_back(f.path().string());
		}
	}
}

std::string ImportsHandler::findFullImportPath(const std::string& module, const std::vector<std::string>& currImportPaths) {
	// First priority: looking for the file in relative directories (current, ../, ...)
	for (auto& path : currImportPaths) {
		std::filesystem::path p(path + module);
		if (std::filesystem::exists(p) && p.string() != g_currFilePath) {
			return path + module;
		}
	}

	// Looking for the imported file in project's import paths
	for (auto& path : g_importPaths) {
		std::filesystem::path p(path + module);
		if (std::filesystem::exists(p) && p.string() != g_currFilePath) {
			return path + module;
		}
	}

	ErrorManager::lexerError(
		ErrorID::E1001_NO_SUCH_MODULE_FOUND, 
		-1, 
		module
	);

	return "";
}

const std::vector<std::string>& ImportsHandler::getCurrFileImportPaths() {
	static std::vector<std::string> result;
	result.clear();
	
	std::vector<std::string> currPath = split(g_currFilePath, '/');
	result.resize(currPath.size());
	for (size_t i = result.size(); i > 0; i--) {
		for (size_t j = 0; j < i; j++) {
			size_t dirToAdd = result.size() - i;
			result[j].append(currPath[dirToAdd]).append("/");
		}
	}

	return result;
}