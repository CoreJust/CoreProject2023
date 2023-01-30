#pragma once
#include <string>
#include "Annotations.h"
#include <llvm/IR/Value.h>

struct Variable {
	std::string name;
	VariableQualities qualities;
	llvm::Value* value;
};