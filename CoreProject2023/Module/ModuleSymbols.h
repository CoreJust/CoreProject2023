#pragma once
#include <vector>
#include "Variable.h"
#include "Function.h"

enum class SymbolType : u8 {
	NO_SYMBOL = 0,
	VARIABLE,
	FUNCTION,
	MODULE
};

class ModuleSymbolsUnit final {
private:
	std::vector<Variable> m_variables;
	std::vector<Function> m_functions;

public:
	void addFunction(std::unique_ptr<FunctionPrototype> prototype, FunctionQualities qualities);
	void addVariable(const std::string& name, VariableQualities qualities, llvm::Value* value);

	SymbolType getSymbolType(const std::string& name) const;

	Function* getFunction(const std::string& name);
	Variable* getVariable(const std::string& name);
};

struct ModuleSymbols {
	ModuleSymbolsUnit publicSymbols;
	ModuleSymbolsUnit publicOnceSymbols;
	ModuleSymbolsUnit privateSymbols;
};