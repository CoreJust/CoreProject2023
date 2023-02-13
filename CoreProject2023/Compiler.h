#pragma once
#include <set>
#include "Project.h"

namespace llvm {
	class TargetMachine;
}

class Compiler final {
private:
	Project& m_project;
	std::set<std::string> m_builtModules;
	std::string m_filesToLink;

public:
	Compiler(Project& project);

	// In such order only
	void buildProject();
	void linkProject();
	void runProject();

private:
	void initAll();
	void initPasses(llvm::TargetMachine* machine = nullptr);

	void preloadSymbols(const std::string& path);
	void compileModules();
	void compileLLVM();

	void addDefaultFunctions();

	std::string genBuildFilePath(const std::string& modulePath, const std::string& extension);
};