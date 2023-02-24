#pragma once
#include "SymbolRef.h"
#include "Type.h"
#include "Function.h"
#include "Variable.h"

// The type's implementation stored in symbol table
// Contains the actual methods, fields, and subtypes
struct TypeNode {
	std::string name;
	TypeQualities qualities;
	std::shared_ptr<Type> type; // in case of an alias
	llvm::Type* llvmType;

	std::vector<Variable> fields;
	std::vector<Function> methods;
	std::vector<std::shared_ptr<TypeNode>> internalTypes;

	TypeNode(
		std::string name, 
		TypeQualities qualities, 
		std::shared_ptr<Type> type, 
		llvm::Type* llvmType,
		std::vector<Variable> fields = { },
		std::vector<Function> methods = { },
		std::vector<std::shared_ptr<TypeNode>> internalTypes = { }
	);

	TypeNode(TypeNode& other);
	TypeNode(TypeNode&& other);

	TypeNode& operator=(TypeNode&& other);

	bool isEquals(std::shared_ptr<TypeNode> other);

	SymbolType getSymbolType(const std::string& name, Visibility visibility, bool isStatic) const;

	// Tries to get a function by name
	// Returns nullptr if nothing found or more than one function with such name exist
	Function* getMethod(const std::string& name, Visibility visibility, bool isStatic);

	// Finds the function with the name and exactly argTypes
	Function* getMethod(
		const std::string& name,
		const std::vector<std::shared_ptr<Type>>& argTypes,
		const std::vector<bool>& isCompileTime,
		bool isStatic
	);

	// Chooses the most suitable function with name for argTypes
	Function* chooseMethod(
		const std::string& name,
		const std::vector<std::shared_ptr<Type>>& argTypes,
		const std::vector<bool>& isCompileTime,
		Visibility visibility,
		bool isStatic
	);

	Variable* getField(const std::string& name, Visibility visibility, bool isStatic);
	std::shared_ptr<TypeNode> getType(const std::string& name, Visibility visibility);

	static std::shared_ptr<Type> genType(std::shared_ptr<TypeNode> typeNode, bool isConst = false);
};

void initBasicTypeNodes();
TypeNode& getBasicTypeNode(BasicType type);
