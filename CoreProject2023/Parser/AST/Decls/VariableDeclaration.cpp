#include "VariableDeclaration.h"
#include <Parser/Visitor/Visitor.h>
#include <Module/LLVMUtils.h>

VariableDeclaration::VariableDeclaration(Variable* var, std::unique_ptr<Expression> value) 
	: m_variable(var), m_value(std::move(value)) {

}

void VariableDeclaration::accept(Visitor* visitor, std::unique_ptr<Declaration>& node) {
	visitor->visit(this, node);
}

void VariableDeclaration::generate() {
	llvm::Value* initializer = nullptr;
	if (m_value) {
		initializer = m_value->generate();
	}

	llvm_utils::createGlobalVariable(*m_variable, initializer);
}
