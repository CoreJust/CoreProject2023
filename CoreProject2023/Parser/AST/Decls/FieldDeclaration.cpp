#include "FieldDeclaration.h"
#include <Parser/Visitor/Visitor.h>
#include <Module/LLVMUtils.h>
#include <Module/Module.h>

FieldDeclaration::FieldDeclaration(
	std::shared_ptr<TypeNode> typeNode,
	Variable* field,
	std::unique_ptr<Expression> value
) : 
	m_typeNode(std::move(typeNode)),
	m_name(field->name),
	m_isStatic(field->qualities.getVariableType() != VariableType::FIELD),
	m_value(std::move(value)) {

}

void FieldDeclaration::accept(Visitor* visitor, std::unique_ptr<Declaration>& node) {
	visitor->visit(this, node);
}

void FieldDeclaration::generate() {
	if (m_isStatic) {
		Variable* field = m_typeNode->getField(m_name, Visibility::PRIVATE, true);
		llvm_utils::createGlobalVariable(*field, m_value.get());
	} else {
		// TODO: default value
	}
}

std::string FieldDeclaration::toString() const {
	static std::string VISIBILITY_STR[4] = { "local ", "private ", "direct_import ", "public " };
	static std::string SAFETY_STR[3] = { "@unsafe\n", "@safe_only\n", "@safe\n" };

	Variable* field = m_typeNode->getField(m_name, Visibility::PRIVATE, m_isStatic);

	std::string result = "";
	if (field->qualities.isThreadLocal()) {
		result += "@thread_local\n";
	}

	result += SAFETY_STR[(u8)field->qualities.getSafety()];

	result += VISIBILITY_STR[(u8)field->qualities.getVisibility()];
	if (field->qualities.getVariableType() != VariableType::FIELD) {
		result += "static ";
	}

	result += field->type->toString();
	result += ' ';
	result += field->name;

	if (m_value) {
		result += " = ";
		result += m_value->toString();
	}

	result += ";\n";
	return result;
}
