#include "Compiler.h"
#include <iostream>
#include <filesystem>
#include <Utils/File.h>
#include <Lexer/ModulePeeker.h>
#include <Lexer/ImportsHandler.h>
#include <Lexer/Lexer.h>
#include <SymbolLoader/SymbolPreloader.h>
#include <SymbolLoader/SymbolLoader.h>
#include <Parser/Parser.h>
#include <Module/LLVMGlobals.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LegacyPassManager.h>
#include "llvm/MC/TargetRegistry.h"
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/FileSystem.h>
#include <llvm\Support\Host.h>
#include <llvm\Target\TargetOptions.h>
#include <llvm\Target\TargetMachine.h>


Compiler::Compiler(Project& project) 
	: m_project(project) {
	g_importPaths = m_project.getImportedPaths();
	initAll();
}

void Compiler::buildProject() {
	preloadSymbols(m_project.getMainFilePath());
	compileModules();
	compileLLVM();
}

void Compiler::linkProject() {
	std::string linkCommand = "clang++.exe -o build/prog.exe"
		+ m_filesToLink
		+ m_project.getAdditionalLinkDirectoriesString();

	system(linkCommand.c_str());
}

void Compiler::runProject() {
	system("C:/Users/egor2/source/repos/CoreProject2023/CoreProject2023/build/prog.exe");
}

void Compiler::initAll() {
	llvm::InitializeAllTargetInfos();
	llvm::InitializeAllTargets();
	llvm::InitializeAllTargetMCs();
	llvm::InitializeAllAsmParsers();
	llvm::InitializeAllAsmPrinters();

	ErrorManager::init(ErrorManager::CONSOLE);

	initPasses();
	initBasicTypeNodes();
}

void Compiler::initPasses(llvm::TargetMachine* machine) {
	llvm::OptimizationLevel optLevel = llvm::OptimizationLevel::O3; // !
	llvm::PassBuilder pb(machine);

	pb.registerModuleAnalyses(*g_moduleAnalysisManager);
	pb.registerCGSCCAnalyses(*g_cgsccAnalysisManager);
	pb.registerFunctionAnalyses(*g_functionAnalysisManager);
	pb.registerLoopAnalyses(*g_loopAnalysisManager);
	pb.crossRegisterProxies(
		*g_loopAnalysisManager,
		*g_functionAnalysisManager,
		*g_cgsccAnalysisManager,
		*g_moduleAnalysisManager
	);

	g_modulePassManager = std::make_unique<llvm::ModulePassManager>(pb.buildPerModuleDefaultPipeline(optLevel));
}

void Compiler::preloadSymbols(const std::string& path) {
	if (m_builtModules.contains(path)) {
		return;
	} else {
		m_builtModules.insert(path);
	}

	g_currFilePath = path;
	g_currFileName = Module::getModuleNameFromPath(path);

	// ModulePeeker
	ModuleRef thisModule;

	{
		ModulePeeker peeker(path);
		thisModule = peeker.load();
	}

	g_moduleList.setCurrentModule(path);

	{
		// Lexer
		std::string program = readFile(path);
		Lexer lexer(program);
		std::vector<Token> toks = lexer.tokenize();

		// Symbols preloading (names)
		SymbolPreloader loader(toks, path);
		loader.loadSymbols();
	}

	for (auto& imp : thisModule->getImports()) {
		preloadSymbols(imp);
	}
}

void Compiler::compileModules() {
	for (auto& module : g_moduleList.getModules()) {
		g_currFilePath = module.getPath();
		g_currFileName = module.getName();

		g_moduleList.setCurrentModule(module.getPath());
		module.loadSymbols();

		// Lexer
		std::vector<Token> toks;

		{
			std::string program = readFile(module.getPath());
			Lexer lexer(program);
			toks = lexer.tokenize();
		}

		// Symbols loading (types)
		{
			SymbolLoader loader(toks, module.getPath());
			loader.loadSymbols();
		}

		// Parser
		g_moduleList.setCurrentModule(module.getPath());
		module.loadAsLLVM();
		g_functionPassManager = std::make_unique<llvm::FunctionPassManager>();
		addDefaultFunctions();
		auto astVec = Parser(toks).parse();
		for (auto& decl : astVec) {
			decl->generate();
		}
	}
}

void Compiler::compileLLVM() {
	// Common settings
	std::string targetTriple = llvm::sys::getDefaultTargetTriple();//"x86_64-pc-windows-msvc";

	llvm::TargetOptions options;
	llvm::Optional<llvm::Reloc::Model> RM = llvm::Optional<llvm::Reloc::Model>();

	std::string error;
	const llvm::Target* target = llvm::TargetRegistry::lookupTarget(targetTriple, error);
	if (!target) {
		std::cout << error;
	}

	llvm::TargetMachine* targetMachine = target->createTargetMachine(targetTriple, "generic", "", options, RM);

	// Compiling llvm modules
	for (auto& module : g_moduleList.getModules()) {
		llvm::Module& llvmModule = module.getLLVMModule();
		llvmModule.setTargetTriple(targetTriple);

		std::cout << "\n\n\n\t\t===================================\n\n\n" << module.getName() << ": \n";
		llvmModule.print(llvm::errs(), nullptr);

		initPasses(targetMachine);
		g_modulePassManager->run(llvmModule, *g_moduleAnalysisManager);

		std::cout << "\n\n\n\nAfter opt " << module.getName() << ": \n";
		llvmModule.print(llvm::errs(), nullptr);

		std::error_code err_code;
		std::string buildFilePath = genBuildFilePath(module.getPath(), ".o");
		llvm::raw_fd_ostream dest(buildFilePath, err_code, llvm::sys::fs::OF_None);
		if (err_code) {
			std::cout << "cannot open file: " << err_code.message();
		}

		m_filesToLink.append(" " + buildFilePath);

		llvmModule.setDataLayout(targetMachine->createDataLayout());
		llvm::legacy::PassManager pass;
		if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, llvm::CGFT_ObjectFile)) {
			std::cout << "targetMachine can't emit a file of this type";
		}

		pass.run(llvmModule);
		dest.flush();
		dest.close();
	}
}

void Compiler::addDefaultFunctions() {

}

std::string Compiler::genBuildFilePath(const std::string& modulePath, const std::string& extension) {
	static std::set<std::string> s_generatedNames;
	std::string moduleName = Module::getModuleNameFromPath(modulePath);
	size_t postfix = 0;
	if (s_generatedNames.contains("build/" + moduleName + extension)) {
		postfix++;
		while (s_generatedNames.contains("build/" + moduleName + std::to_string(postfix) + extension)) {
			postfix++;
		}
	}

	std::string generatedName;
	if (postfix) {
		generatedName = "build/" + moduleName + std::to_string(postfix) + extension;
	} else {
		generatedName = "build/" + moduleName + extension;
	}

	s_generatedNames.insert(generatedName);
	return generatedName;
}
