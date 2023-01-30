#include "Compiler.h"
#include <iostream>
#include <filesystem>
#include <Utils/File.h>
#include <Lexer/Lexer.h>
#include <SymbolLoader/SymbolLoader.h>
#include <Parser/Parser.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include "llvm/MC/TargetRegistry.h"
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/FileSystem.h>
#include <llvm\Support\Host.h>
#include <llvm\Target\TargetOptions.h>
#include <llvm\Target\TargetMachine.h>

Compiler::Compiler(Project& project) 
	: m_project(project) {
	initAll();
}

void Compiler::buildProject() {
	handleFrontEnd();
	compileLLVM();
}

void Compiler::linkProject() {
	std::string linkCommand = "clang++.exe -o build/prog.exe" + m_filesToLink;
	system(linkCommand.c_str());
}

void Compiler::runProject() {
	//system("build/prog.exe");
}

void Compiler::initAll() {
	llvm::InitializeAllTargetInfos();
	llvm::InitializeAllTargets();
	llvm::InitializeAllTargetMCs();
	llvm::InitializeAllAsmParsers();
	llvm::InitializeAllAsmPrinters();

	ErrorManager::init(ErrorManager::CONSOLE);
}

void Compiler::handleFrontEnd() {
	compileModule(m_project.getMainFilePath());
}

void Compiler::compileModule(const std::string& path) {
	if (m_builtModules.contains(path)) {
		return;
	} else {
		m_builtModules.insert(path);
	}

	std::string currFilePath = Module::getModulePathWithoutName(path);
	g_currFilePath = currFilePath;

	// Lexer
	ModuleQualities qualities;
	std::vector<std::string> imports;
	std::vector<Token> toks;

	{
		std::string program = readFile(path);
		Lexer lexer(program);
		qualities = lexer.handleModuleQualities();
		imports = lexer.handleImports();
		toks = lexer.tokenize();
	}
	
	// Adding module to module table
	ModuleRef thisModule;
	{
		std::string moduleName = Module::getModuleNameFromPath(path);
		Module module(moduleName, path, qualities, imports);
		thisModule = g_moduleList.addModule(module);
		g_moduleList.setCurrentModule(path);
	}

	// Symbols loading
	{
		SymbolLoader loader(toks, qualities, path);
		loader.loadSymbols();
	}

	// Building imported modules
	for (auto& imp : imports) {
		compileModule(imp);
	}

	g_currFilePath = currFilePath;

	// Parser
	thisModule->loadSymbols();
	g_moduleList.setCurrentModule(path);
	g_functionPassManager = std::make_unique<llvm::legacy::FunctionPassManager>(&thisModule->getLLVMModule());
	addDefaultFunctions();
	auto astVec = Parser(std::move(toks)).parse();
	for (auto& decl : astVec) {
		decl->generate();
	}
}

void Compiler::compileLLVM() {
	auto targetTriple = llvm::sys::getDefaultTargetTriple();//"x86_64-pc-windows-msvc";
	for (auto& module : g_moduleList.getModules()) {
		llvm::Module& llvmModule = module.getLLVMModule();
		llvmModule.setTargetTriple(targetTriple);

		std::cout << module.getName() << ": \n";
		llvmModule.print(llvm::errs(), nullptr);

		llvm::TargetOptions options;
		auto RM = llvm::Optional<llvm::Reloc::Model>();

		std::error_code err_code;
		std::string buildFilePath = getBuildFilePath(module.getPath(), ".o");
		llvm::raw_fd_ostream dest(buildFilePath, err_code, llvm::sys::fs::OF_None);
		if (err_code)
			std::cout << "cannot open file: " << err_code.message();

		m_filesToLink.append(" " + buildFilePath);

		std::string error;
		auto target = llvm::TargetRegistry::lookupTarget(targetTriple, error);
		if (!target)
			std::cout << error;

		auto targetMachine = target->createTargetMachine(targetTriple, "generic", "", options, RM);
		llvmModule.setDataLayout(targetMachine->createDataLayout());
		if (targetMachine->addPassesToEmitFile(*g_modulePassManager, dest, nullptr, llvm::CGFT_ObjectFile))
			std::cout << "targetMachine can't emit a file of this type";

		g_modulePassManager->run(llvmModule);
		dest.flush();
		dest.close();
	}
}

void Compiler::addDefaultFunctions() {
	llvm::Module* llvmModule = &g_module->getLLVMModule();

	llvm::ArrayRef<llvm::Type*> system_params(llvm::Type::getInt8PtrTy(g_context));
	llvm::FunctionType* system_type = llvm::FunctionType::get(llvm::Type::getInt32Ty(g_context), system_params, false);
	llvm::Function* system_f = llvm::Function::Create(system_type, llvm::Function::ExternalLinkage, "system", llvmModule);
}

std::string Compiler::getBuildFilePath(const std::string& modulePath, const std::string& extension) {
	std::string moduleName = Module::getModuleNameFromPath(modulePath);
	size_t postfix = 0;
	if (std::filesystem::exists("build/" + moduleName + extension)) {
		postfix++;
		while (std::filesystem::exists("build/" + moduleName + std::to_string(postfix) + extension)) {
			postfix++;
		}
	}

	if (postfix) {
		return "build/" + moduleName + std::to_string(postfix) + extension;
	}

	return "build/" + moduleName + extension;
}
