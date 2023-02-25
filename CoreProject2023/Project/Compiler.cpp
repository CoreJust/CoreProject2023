#include "Compiler.h"
#include <iostream>
#include <filesystem>
#include <Utils/File.h>
#include <Utils/String.h>
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
	g_importPaths = m_project.getSettings().additionalImportDirs;
	initAll();
}

void Compiler::buildProject() {
	for (auto& path : m_project.getSettings().compiledCoreModules) {
		preloadSymbols(path);
	}

	compileModules();
	compileLLVM();
}

void Compiler::linkProject() {
	for (auto& path : m_project.getSettings().additionalLinkedObjectFiles) {
		m_filesToLink += ' ';
		m_filesToLink += path;
	}

	std::string linkCommand = "clang++.exe -o "
		+ m_project.getSettings().output.getOutputFile(CompilerOutput::ExecutableData)
		+ m_filesToLink;

	system(linkCommand.c_str());
}

void Compiler::runProject() {
	ASSERT(m_project.getSettings().compilationMode == CompilationMode::Program, "cannot run a library");
	system(m_project.getSettings().output.getOutputFile(CompilerOutput::ExecutableData).c_str());
}

void Compiler::initAll() {
	llvm::InitializeAllTargetInfos();
	llvm::InitializeAllTargets();
	llvm::InitializeAllTargetMCs();
	llvm::InitializeAllAsmParsers();
	llvm::InitializeAllAsmPrinters();

	initPasses();
	initBasicTypeNodes();
}

void Compiler::initPasses(llvm::TargetMachine* machine) {
	llvm::OptimizationLevel optLevel;
	switch (m_project.getSettings().optLevel) {
		case OptimizationLevel::O0: optLevel = llvm::OptimizationLevel::O0; break;
		case OptimizationLevel::O1: optLevel = llvm::OptimizationLevel::O1; break;
		case OptimizationLevel::O2: optLevel = llvm::OptimizationLevel::O2; break;
		case OptimizationLevel::O3: optLevel = llvm::OptimizationLevel::O3; break;
	default: optLevel = llvm::OptimizationLevel::O0; break;
	}

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

			if (m_project.getSettings().output.getOutputMode(CompilerOutput::Lexer) != CompilerOutput::NoOut) {
				printTokens(toks);
			}
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

		std::vector<std::unique_ptr<Declaration>> astVec = Parser(toks).parse();
		if (m_project.getSettings().output.getOutputMode(CompilerOutput::ASTBeforeOpt) != CompilerOutput::NoOut) {
			printAst(astVec, false);
		}

		// TODO: add visitors

		if (m_project.getSettings().output.getOutputMode(CompilerOutput::ASTAfterOpt) != CompilerOutput::NoOut) {
			printAst(astVec, true);
		}

		for (auto& decl : astVec) {
			decl->generate();
		}
	}
}

void Compiler::compileLLVM() {
	// Common settings
	std::string targetTriple = getTargetTriple();

	llvm::TargetOptions options;
	llvm::Optional<llvm::Reloc::Model> RM = llvm::Optional<llvm::Reloc::Model>();

	std::string error;
	const llvm::Target* target = llvm::TargetRegistry::lookupTarget(targetTriple, error);
	if (!target) {
		std::cout << error;
	}

	llvm::TargetMachine* targetMachine = target->createTargetMachine(targetTriple, "generic", "", options, RM);

	// Compiling llvm modules
	if (m_project.getSettings().compilationMode == CompilationMode::Program) {
		ModuleRef mainModule = g_moduleList.getModule(m_project.getSettings().compiledCoreModules[0]);

		mainModule->getLLVMModule().setTargetTriple(targetTriple);
		std::string buildFilePath = m_project.getSettings().output.getOutputFile(CompilerOutput::ObjectData);

		compileLLVMModule(
			&mainModule->getLLVMModule(),
			buildFilePath,
			targetMachine
		);
	} else { // Compiling as a library
		for (auto& module : g_moduleList.getModules()) {
			llvm::Module& llvmModule = module.getLLVMModule();
			llvmModule.setTargetTriple(targetTriple);

			compileLLVMModule(
				&llvmModule,
				genBuildFilePath(module.getPath(), ".o"),
				targetMachine
			);
		}
	} 
}

void Compiler::compileLLVMModule(
	llvm::Module* llvmModule,
	const std::string& buildFilePath,
	llvm::TargetMachine* targetMachine
) {
	if (m_project.getSettings().output.getOutputMode(CompilerOutput::IRBeforeOpt) != CompilerOutput::NoOut) {
		printIR(llvmModule, false);
	}

	// LLVM optimization
	if (m_project.getSettings().optLevel != OptimizationLevel::O0) {
		initPasses(targetMachine);
		g_modulePassManager->run(*llvmModule, *g_moduleAnalysisManager);

		if (m_project.getSettings().output.getOutputMode(CompilerOutput::IRAfterOpt) != CompilerOutput::NoOut) {
			printIR(llvmModule, true);
		}
	}

	std::error_code err_code;
	llvm::raw_fd_ostream dest(buildFilePath, err_code, llvm::sys::fs::OF_None);
	if (err_code) {
		std::cout << "cannot open file: " << err_code.message();
	}

	m_filesToLink.append(" " + buildFilePath);

	llvmModule->setDataLayout(targetMachine->createDataLayout());
	llvm::legacy::PassManager pass;
	if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, llvm::CGFT_ObjectFile)) {
		std::cout << "targetMachine can't emit a file of this type";
	}

	pass.run(*llvmModule);
	dest.flush();
	dest.close();
}

void Compiler::addDefaultFunctions() {

}

std::string Compiler::getTargetTriple() {
	std::string result = llvm::sys::getDefaultTargetTriple();
	std::vector<std::string> settings = split(result, '-');


	return result;
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

void Compiler::printTokens(const std::vector<Token>& toks) {
	std::string text = "Tokens from file " + g_currFilePath + "\n";
	for (auto& tok : toks) {
		text += tok.toString();
		text += '\n';
	}

	print(text, CompilerOutput::Lexer);
}

void Compiler::printAst(const std::vector<std::unique_ptr<Declaration>>& ast, bool isOptimized) {
	std::string text = "AST from file " + g_currFilePath + "\n";
	for (auto& decl : ast) {
		text += decl->toString();
	}

	print(text, isOptimized ? CompilerOutput::ASTAfterOpt : CompilerOutput::ASTBeforeOpt);
}

void Compiler::printIR(llvm::Module* ir, bool isOptimized) {
	CompilerOutput::OutputStage stage = isOptimized ? CompilerOutput::ASTAfterOpt : CompilerOutput::ASTBeforeOpt;
	if (m_project.getSettings().output.getOutputMode(stage) == CompilerOutput::File) {
		std::string fileName = m_project.getSettings().output.getOutputFile(stage);
		std::error_code err_code;
		llvm::raw_fd_ostream file(fileName, err_code, llvm::sys::fs::OF_None);

		if (isOptimized) {
			file << "Optimized ";
		}

		file << "LLVM IR from file " << g_currFilePath << '\n';
		ir->print(file, nullptr);
		file << "\n\n";
	} else {
		if (isOptimized) {
			llvm::errs() << "Optimized ";
		}

		llvm::errs() << "LLVM IR from file " << g_currFilePath << '\n';
		ir->print(llvm::errs(), nullptr);
		llvm::errs() << "\n\n";
	}
}

void Compiler::print(const std::string& text, CompilerOutput::OutputStage stage) {
	if (m_project.getSettings().output.getOutputMode(stage) == CompilerOutput::Console) {
		std::cout << text << std::endl << std::endl;
	} else { // File
		std::ofstream file(m_project.getSettings().output.getOutputFile(stage));
		file << text << std::endl << std::endl;
		file.close();
	}
}
