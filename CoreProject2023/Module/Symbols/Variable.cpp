#include "Variable.h"

Variable::Variable(
	std::string name, 
	std::unique_ptr<Type> type, 
	VariableQualities qualities, 
	llvm::Value* value
) : 
	name(std::move(name)), 
	type(std::move(type)), 
	qualities(qualities), 
	value(value) {

}

Variable::Variable(Variable& other)
	: Variable(other.name, other.type->copy(), other.qualities, other.value) {

}
