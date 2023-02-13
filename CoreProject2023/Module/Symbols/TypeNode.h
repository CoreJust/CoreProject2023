#pragma once
#include "Type.h"
#include "Function.h"
#include "Variable.h"

// The type's implementation stored in symbol table
// Contains the actual methods, fields, and subtypes
struct TypeNode {
	std::string name;
	TypeQualities qualities;
	std::unique_ptr<Type> type; // in case of an alias
	llvm::Type* llvmType;

	std::vector<Variable> fields;
	std::vector<Function> methods;
	std::vector<std::unique_ptr<TypeNode>> internalTypes;

	TypeNode(
		std::string name, 
		TypeQualities qualities, 
		std::unique_ptr<Type> type, 
		llvm::Type* llvmType,
		std::vector<Variable> fields = { },
		std::vector<Function> methods = { },
		std::vector<std::unique_ptr<TypeNode>> internalTypes = { }
	);

	TypeNode(TypeNode& other);
	TypeNode(TypeNode&& other);

	TypeNode& operator=(TypeNode&& other);

	static std::unique_ptr<Type> genType(std::shared_ptr<TypeNode> typeNode, bool isConst = false);
};

void initBasicTypeNodes();
TypeNode& getBasicTypeNode(BasicType type);
