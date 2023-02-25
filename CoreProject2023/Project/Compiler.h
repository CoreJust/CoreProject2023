#pragma once
#include <set>
#include <memory>
#include "Project.h"

struct Token;
class Declaration;

namespace llvm {
	class TargetMachine;
	class Module;
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
	void compileLLVMModule(
		llvm::Module* llvmModule,
		const std::string& buildFilePath,
		llvm::TargetMachine* targetMachine
	);

	void addDefaultFunctions();

	std::string getTargetTriple();
	std::string genBuildFilePath(const std::string& modulePath, const std::string& extension);

private:
	void printTokens(const std::vector<Token>& toks);
	void printAst(const std::vector<std::unique_ptr<Declaration>>& ast, bool isOptimized);
	void printIR(llvm::Module* ir, bool isOptimized);

	void print(const std::string& text, CompilerOutput::OutputStage stage);
};