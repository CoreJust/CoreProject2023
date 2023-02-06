#pragma once
#include <string>
#include <llvm/IR/Value.h>
#include "Annotations.h"
#include "Type.h"

struct Variable {
	std::string name;
	std::unique_ptr<Type> type;
	VariableQualities qualities;
	llvm::Value* value;

	Variable(std::string name, std::unique_ptr<Type> type, VariableQualities qualities, llvm::Value* value);
	Variable(Variable&&) = default;
	Variable(Variable& other);
};