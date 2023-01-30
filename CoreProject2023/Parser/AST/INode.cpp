#include "INode.h"

llvm::LLVMContext g_context;
std::unique_ptr<llvm::IRBuilder<>> g_builder = std::make_unique<llvm::IRBuilder<>>(g_context);
std::unique_ptr<llvm::legacy::FunctionPassManager> g_functionPassManager;
std::unique_ptr<llvm::legacy::PassManager> g_modulePassManager = std::make_unique<llvm::legacy::PassManager>();;