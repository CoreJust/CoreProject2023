#pragma once
#include <vector>
#include <map>
#include "Symbols/SymbolRef.h"
#include "Symbols/Variable.h"
#include "Symbols/Function.h"
#include "Symbols/TypeNode.h"

class ModuleSymbols;

class ModuleSymbolsUnit final {
	friend class ModuleSymbols;

private:
	std::vector<Variable> m_variables;
	std::vector<Function> m_functions;
	std::vector<Function> m_constructors;
	std::vector<Function> m_operators;
	std::vector<std::shared_ptr<TypeNode>> m_types;

public:
	void addType(std::shared_ptr<TypeNode> type);
	void addFunction(FunctionPrototype prototype);
	void addConstructor(FunctionPrototype prototype);
	void addDestructor(FunctionPrototype prototype);
	void addOperator(FunctionPrototype prototype);

	// Intended for adding function duplicates to modules importing the function
	void addFunction(FunctionPrototype prototype, llvm::Function* value);
	void addConstructor(FunctionPrototype prototype, llvm::Function* value);
	void addOperator(FunctionPrototype prototype, llvm::Function* value);

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

	// Chooses the most suitable constructor of type for argTypes
	Function* chooseConstructor(
		const std::unique_ptr<Type>& type,
		const std::vector<std::unique_ptr<Type>>& argTypes,
		const std::vector<bool>& isCompileTime,
		bool isImplicit
	);

	// Chooses the most suitable operator with operator-name for argTypes
	Function* chooseOperator(
		const std::string& name,
		const std::vector<std::unique_ptr<Type>>& argTypes,
		const std::vector<bool>& isCompileTime,
		bool mustReturnReference
	);

	Variable* getVariable(const std::string& name);
	std::shared_ptr<TypeNode> getType(const std::string& name);

	std::vector<Variable>& getVariables();
	std::vector<Function>& getFunctions();
	std::vector<Function>& getConstructors();
	std::vector<Function>& getOperators();
	std::vector<std::shared_ptr<TypeNode>>& getTypes();

	bool isEmpty() const;
};

class ModuleSymbols {
private:
	// ordered list of SymbolRefs refering to corresponding symbols in the unit
	std::vector<SymbolRef> m_symbolRefs;

public:
	ModuleSymbolsUnit publicSymbols;
	ModuleSymbolsUnit publicOnceSymbols;
	ModuleSymbolsUnit privateSymbols;

public:
	// deprecated
	void sortSymbolRefs();

	void addType(Visibility visibility, std::shared_ptr<TypeNode> type, u64 tokenPos);
	void addFunction(Visibility visibility, FunctionPrototype prototype, u64 tokenPos);
	void addConstructor(Visibility visibility, FunctionPrototype prototype, u64 tokenPos);
	void addOperator(Visibility visibility, FunctionPrototype prototype, u64 tokenPos);

	void addVariable(
		Visibility visibility,
		const std::string& name,
		VariableQualities qualities,
		u64 tokenPos
	);

	Function* getFunction(u64 tokenPos); // as well as constructor, or operator
	Variable* getVariable(u64 tokenPos);
	std::shared_ptr<TypeNode> getType(u64 tokenPos);

private:
	SymbolRef& getSymbolRefByTokenPos(u64 from, u64 to, u64 tokenPos);

public:
	ModuleSymbolsUnit& getModuleSymbolsUnit(Visibility visibility);
};