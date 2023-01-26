#include <iostream>
#include <Utils/File.h>
#include <Lexer/Lexer.h>
#include <Parser/Parser.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include "llvm/IR/LegacyPassManager.h"
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include "llvm/MC/TargetRegistry.h"
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/FileSystem.h>
#include <llvm\Support\Host.h>
#include <llvm\Target\TargetOptions.h>
#include <llvm\Target\TargetMachine.h>
#include <llvm\Transforms\IPO.h>
#include <llvm\Transforms\Utils\BasicBlockUtils.h>
#include <llvm/Transforms/Scalar.h>
#include <Parser/AST/ValueExpr.h>

// TODO: Add simple parser with functions and int values and compilation, clean main.cpp
// Long term TODO: add error/warning codes (like E1011, W1112, etc), project settings file

void init() {
	llvm::InitializeAllTargetInfos();
	llvm::InitializeAllTargets();
	llvm::InitializeAllTargetMCs();
	llvm::InitializeAllAsmParsers();
	llvm::InitializeAllAsmPrinters();

	ErrorManager::init(ErrorManager::CONSOLE);
	g_currFilePath = "C:/Users/egor2/source/repos/CoreProject2023/examples/";
}

int main() {
	init();
	std::string program = readFile("../examples/test.core");
	Lexer lexer(program);
	auto imports = lexer.handleImports();
	auto toks = lexer.tokenize();
	auto expr = Parser(std::move(toks)).parse();

	// Experiments, to clean
	llvm::Module module("main", g_context);
	
	auto targetTriple = llvm::sys::getDefaultTargetTriple();//"x86_64-pc-windows-msvc";
	module.setTargetTriple(targetTriple);

	llvm::ArrayRef<llvm::Type*> params(llvm::Type::getInt32Ty(g_context));
	llvm::FunctionType* type = llvm::FunctionType::get(llvm::Type::getInt32Ty(g_context), params, false);
	llvm::Function* puts_f = llvm::Function::Create(type, llvm::Function::ExternalLinkage, "putchar", module);

	llvm::ArrayRef<llvm::Type*> system_params(llvm::Type::getInt8PtrTy(g_context));
	llvm::FunctionType* system_type = llvm::FunctionType::get(llvm::Type::getInt32Ty(g_context), system_params, false);
	llvm::Function* system_f = llvm::Function::Create(system_type, llvm::Function::ExternalLinkage, "system", module);

	llvm::ArrayRef<llvm::Type*> main_params;
	llvm::FunctionType* main_type = llvm::FunctionType::get(llvm::Type::getInt32Ty(g_context), main_params, false);
	llvm::Function* main_f = llvm::Function::Create(main_type, llvm::Function::ExternalLinkage, "main", module);

	llvm::BasicBlock* bb = llvm::BasicBlock::Create(g_context, "entry", main_f);
	g_builder->SetInsertPoint(bb);
	g_builder->CreateCall(type, puts_f, llvm::ArrayRef<llvm::Value*>({ expr->generate() }));
	g_builder->CreateCall(system_type, system_f, llvm::ArrayRef<llvm::Value*>({ g_builder->CreateGlobalStringPtr("pause") }));
	g_builder->CreateRet(llvm::ConstantInt::get(g_context, llvm::APInt(32, 0, true)));

	module.print(llvm::errs(), nullptr);

	llvm::TargetOptions options;
	auto RM = llvm::Optional<llvm::Reloc::Model>();

	std::error_code err_code;
	llvm::raw_fd_ostream dest("build/prog.o", err_code, llvm::sys::fs::OF_None);
	if (err_code)
		std::cout << "cannot open file: " << err_code.message();

	std::string error;
	auto target = llvm::TargetRegistry::lookupTarget(targetTriple, error);
	if (!target)
		std::cout << error;

	auto targetMachine = target->createTargetMachine(targetTriple, "generic", "", options, RM);
	module.setDataLayout(targetMachine->createDataLayout());
	llvm::legacy::PassManager pass;
	if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, llvm::CGFT_ObjectFile))
		std::cout << "targetMachine can't emit a file of this type";

	pass.run(module);
	dest.flush();
	dest.close();

	system("clang++.exe -o build/prog.exe build/prog.o");

	return 0;
}