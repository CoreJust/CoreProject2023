#include "VariableExpr.h"
#include <Parser/Visitor/Visitor.h>
#include <Module/Module.h>

VariableExpr::VariableExpr(Variable* variable)
	: m_name(variable->name) {
	m_type = variable->type->copy();
	m_isRVal = true;
}

void VariableExpr::accept(Visitor* visitor, std::unique_ptr<Expression>& node) {
	visitor->visit(this, node);
}

llvm::Value* VariableExpr::generate() {
	llvm::Value* varVal = g_module->getVariable(m_name)->value;
	llvm::Value* result = g_builder->CreateLoad(m_type->to_llvm(), varVal);

	if (m_type->basicType == BasicType::REFERENCE) {
		result = g_builder->CreateLoad(((PointerType*)m_type.get())->elementType->to_llvm(), result);
	}

	return result;
}

llvm::Value* VariableExpr::generateRValue() {
	llvm::Value* result = g_module->getVariable(m_name)->value;

	if (m_type->basicType == BasicType::REFERENCE) {
		result = g_builder->CreateLoad(m_type->to_llvm(), result);
	}

	return result;
}
