#pragma once
#include <map>
#include <llvm/IR/Value.h>

struct Variable;

// Allows to get get the llvm::Value* of a global object as in the current module
// Returns the original value for the module it is located in and the external value (variable/function) for other modules
class LLVMVariableManager {
	std::map<std::string, llvm::Value*> m_variableValues;
	llvm::Value* m_originalValue = nullptr;

public:
	void setInitialValue(llvm::Value* varValue);

	// Generates external variable if there is no such or returns already existing one
	llvm::Value* getVariableValueForCurrentModule(Variable* var);

	llvm::Value* getOriginalValue();
};