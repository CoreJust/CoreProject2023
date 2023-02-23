#pragma once
#include <string>
#include "Annotations.h"
#include "Type.h"
#include "LLVMVariableManager.h"

struct Variable {
	std::string name;
	std::unique_ptr<Type> type;
	VariableQualities qualities;
	std::shared_ptr<LLVMVariableManager> valueManager = std::make_shared<LLVMVariableManager>();

	Variable(
		std::string name, 
		std::unique_ptr<Type> type, 
		VariableQualities qualities, 
		std::shared_ptr<LLVMVariableManager> value
	);

	Variable(Variable&&) = default;
	Variable(Variable& other);

	llvm::Value* getValue();
};