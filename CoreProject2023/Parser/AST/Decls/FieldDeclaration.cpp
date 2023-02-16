#include "FieldDeclaration.h"
#include <Parser/Visitor/Visitor.h>
#include <Module/LLVMUtils.h>
#include <Module/Module.h>

FieldDeclaration::FieldDeclaration(Variable* field, std::unique_ptr<Expression> value)
	: m_field(std::move(field)), m_value(std::move(value)) {

}

void FieldDeclaration::accept(Visitor* visitor, std::unique_ptr<Declaration>& node) {
	visitor->visit(this, node);
}

void FieldDeclaration::generate() {
	if (m_field->qualities.getVariableType() != VariableType::FIELD) {
		llvm_utils::createGlobalVariable(*m_field, m_value.get());
	} else {
		// TODO: default value
	}
}
