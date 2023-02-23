#include "Variable.h"

Variable::Variable(
	std::string name, 
	std::unique_ptr<Type> type, 
	VariableQualities qualities, 
	std::shared_ptr<LLVMVariableManager> value
) : 
	name(std::move(name)), 
	type(std::move(type)), 
	qualities(qualities),
	valueManager(value) {
	if (!valueManager) {
		valueManager = std::make_shared<LLVMVariableManager>();
	}
}

Variable::Variable(Variable& other)
	: Variable(other.name, other.type->copy(), other.qualities, other.valueManager) {

}

llvm::Value* Variable::getValue() {
	return valueManager->getVariableValueForCurrentModule(this);
}
