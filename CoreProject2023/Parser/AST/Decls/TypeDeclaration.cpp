#include "TypeDeclaration.h"
#include <Parser/Visitor/Visitor.h>

std::shared_ptr<TypeNode> g_type;


TypeDeclaration::TypeDeclaration(
	std::shared_ptr<TypeNode> type,
	std::vector<std::unique_ptr<Declaration>> fields,
	std::vector<std::unique_ptr<Declaration>> methods
) :
	m_typeNode(std::move(type)),
	m_fields(std::move(fields)),
	m_methods(std::move(methods)) {

}

void TypeDeclaration::accept(Visitor* visitor, std::unique_ptr<Declaration>& node) {
	visitor->visit(this, node);
}

void TypeDeclaration::generate() {
	std::shared_ptr<TypeNode> previousType = g_type;
	g_type = m_typeNode;

	for (auto& field : m_fields) {
		field->generate();
	}

	for (auto& method : m_methods) {
		method->generate();
	}

	g_type = previousType;
}

std::string TypeDeclaration::toString() const {
	std::string result = m_typeNode->type->basicType == BasicType::STRUCT
		? "struct "
		: "class ";

	result += m_typeNode->name;
	result += " {\n";
	for (auto& field : m_fields) {
		result += field->toString();
	}

	result += "\n\n";

	for (auto& method : m_methods) {
		result += method->toString();
	}

	result += "\n}\n\n";
	return result;
}
