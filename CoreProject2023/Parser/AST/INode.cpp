#include "INode.h"

llvm::LLVMContext g_context;
std::unique_ptr<llvm::IRBuilder<>> g_builder = std::make_unique<llvm::IRBuilder<>>(g_context);