#pragma once
#include "FunctionPrototype.h"
#include "LLVMFunctionManager.h"

struct Function {
	FunctionPrototype prototype;
	std::shared_ptr<LLVMFunctionManager> functionManager = std::make_shared<LLVMFunctionManager>();
	
	Function(FunctionPrototype proto, std::shared_ptr<LLVMFunctionManager> manager);

	llvm::Function* getValue();
};