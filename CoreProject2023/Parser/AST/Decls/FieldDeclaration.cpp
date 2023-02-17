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
