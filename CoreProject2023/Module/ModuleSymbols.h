#pragma once
#include <vector>
#include "Symbols/Variable.h"
#include "Symbols/Function.h"
#include "Symbols/TypeNode.h"

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
	void addFunction(FunctionPrototype prototype);

	// Intended for adding function duplicates to modules importing the function
	void addFunction(FunctionPrototype prototype, llvm::Function* value);

	void addVariable(
		const std::string& name, 
		std::unique_ptr<Type> type, 
		VariableQualities qualities, 
		llvm::Value* value
	);

	SymbolType getSymbolType(const std::string& name) const;

	// Tries to get a function by name
	// Returns nullptr if nothing found or more than one function with such name exist
	Function* getFunction(const std::string& name);

	// Finds the function with the name and exactly argTypes
	Function* getFunction(
		const std::string& name,
		const std::vector<std::unique_ptr<Type>>& argTypes,
		const std::vector<bool>& isCompileTime
	);

	// Chooses the most suitable function with name for argTypes
	Function* chooseFunction(
		const std::string& name,
		const std::vector<std::unique_ptr<Type>>& argTypes,
		const std::vector<bool>& isCompileTime
	);

	Variable* getVariable(const std::string& name);
	TypeNode* getType(const std::string& name);

	std::vector<Variable>& getVariables();
	std::vector<Function>& getFunctions();

	bool isEmpty() const;
};

struct ModuleSymbols {
	ModuleSymbolsUnit publicSymbols;
	ModuleSymbolsUnit publicOnceSymbols;
	ModuleSymbolsUnit privateSymbols;
};