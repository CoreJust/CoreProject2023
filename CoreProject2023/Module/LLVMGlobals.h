#pragma once
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>

extern llvm::LLVMContext g_context;
extern std::unique_ptr<llvm::IRBuilder<>> g_builder;

extern std::unique_ptr<llvm::FunctionPassManager> g_functionPassManager;
extern std::unique_ptr<llvm::ModulePassManager> g_modulePassManager;

extern std::unique_ptr<llvm::ModuleAnalysisManager> g_moduleAnalysisManager;
extern std::unique_ptr<llvm::FunctionAnalysisManager> g_functionAnalysisManager;
extern std::unique_ptr<llvm::LoopAnalysisManager> g_loopAnalysisManager;
extern std::unique_ptr<llvm::CGSCCAnalysisManager> g_cgsccAnalysisManager;