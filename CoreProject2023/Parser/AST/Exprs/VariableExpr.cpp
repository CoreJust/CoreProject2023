#include "VariableExpr.h"
#include <Parser/Visitor/Visitor.h>

VariableExpr::VariableExpr(Variable* variable)
	: m_variable(variable) {

}

void VariableExpr::accept(Visitor* visitor, std::unique_ptr<Expression>& node) {
	visitor->visit(this, node);
}

llvm::Value* VariableExpr::generate() {
	llvm::Value* varVal = m_variable->value;
	llvm::Value* result = g_builder->CreateLoad(llvm::Type::getInt32Ty(g_context), varVal);

	return result;
}
