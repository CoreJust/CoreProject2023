#pragma once
#include <vector>
#include "Variable.h"
#include "Function.h"
#include "TypeNode.h"

enum class SymbolType : u8 {
	NO_SYMBOL = 0,
	VARIABLE,
	FUNCTION,
	TYPE,
	MODULE
};

class ModuleSymbolsUnit final {
private:
	std::vector<Variable> m_variables;
	std::vector<Function> m_functions;
	std::vector<TypeNode> m_types;

public:
	void addType(TypeNode type);
	void addFunction(FunctionPrototype prototype, FunctionQualities qualities);

	// Intended for adding function duplicates to modules importing the function
	void addFunction(FunctionPrototype prototype, FunctionQualities qualities, llvm::Function* value);

	void addVariable(const std::string& name, std::unique_ptr<Type> type, VariableQualities qualities, llvm::Value* value);

	SymbolType getSymbolType(const std::string& name) const;

	Function* getFunction(const std::string& name);
	Variable* getVariable(const std::string& name);
	TypeNode* getType(const std::string& name);

	std::vector<Variable>& getVariables();
	std::vector<Function>& getFunctions();
};

struct ModuleSymbols {
	ModuleSymbolsUnit publicSymbols;
	ModuleSymbolsUnit publicOnceSymbols;
	ModuleSymbolsUnit privateSymbols;
};