#include "VariableExpr.h"
#include <Parser/Visitor/Visitor.h>
#include <Module/Module.h>
#include <Module/LLVMGlobals.h>

VariableExpr::VariableExpr(std::string moduleName, Variable* variable)
	: m_isStaticTypeMember(false), m_moduleName(std::move(moduleName)), m_name(variable->name) {
	m_type = variable->type->copy();
	m_isRVal = true;
}

VariableExpr::VariableExpr(std::shared_ptr<TypeNode> typeNode, Variable* variable)
	: m_isStaticTypeMember(true), m_typeNode(std::move(typeNode)), m_name(variable->name) {
	m_type = variable->type->copy();
	m_isRVal = true;
}

VariableExpr::~VariableExpr() {
	m_name.~basic_string();
	m_type.~unique_ptr();
	if (m_isStaticTypeMember) {
		m_typeNode.~shared_ptr();
	} else {
		m_moduleName.~basic_string();
	}
}

void VariableExpr::accept(Visitor* visitor, std::unique_ptr<Expression>& node) {
	visitor->visit(this, node);
}

llvm::Value* VariableExpr::generate() {
	llvm::Value* varVal = m_isStaticTypeMember ?
		m_typeNode->getField(m_name, Visibility::PRIVATE, true)->value
		: g_module->getVariable(m_moduleName, m_name)->value;

	llvm::Value* result = g_builder->CreateLoad(m_type->to_llvm(), varVal);

	if (m_type->basicType == BasicType::REFERENCE) {
		result = g_builder->CreateLoad(((PointerType*)m_type.get())->elementType->to_llvm(), result);
	}

	return result;
}

llvm::Value* VariableExpr::generateRValue() {
	llvm::Value* result = m_isStaticTypeMember ?
		m_typeNode->getField(m_name, Visibility::PRIVATE, true)->value
		: g_module->getVariable(m_moduleName, m_name)->value;

	if (m_type->basicType == BasicType::REFERENCE) {
		result = g_builder->CreateLoad(m_type->to_llvm(), result);
	}

	return result;
}
