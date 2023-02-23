#pragma once
#include <map>
#include <llvm/IR/Function.h>

class FunctionPrototype;

// Allows to get get the llvm::Function* of a function
// Returns the original value for the module it is located in and the external function for other modules
class LLVMFunctionManager {
	std::map<std::string, llvm::Function*> m_functionValues;
	llvm::Function* m_originalValue = nullptr;

public:
	void setInitialValue(llvm::Function* funcValue);

	// Generates external variable if there is no such or returns already existing one
	llvm::Function* getFunctionValueForCurrentModule(FunctionPrototype* func);

	llvm::Function* getOriginalValue();
};