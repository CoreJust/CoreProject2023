#pragma once
#include "Declaration.h"
#include "FieldDeclaration.h"
#include "MethodDeclaration.h"
#include "../States/Statement.h"
#include <Module/Symbols/TypeNode.h>

class TypeDeclaration : public Declaration {
	FRIEND_CLASS_VISITORS

public:
	TypeDeclaration(
		std::shared_ptr<TypeNode> type,
		std::vector<std::unique_ptr<Declaration>> fields,
		std::vector<std::unique_ptr<Declaration>> methods
	);

	void accept(Visitor* visitor, std::unique_ptr<Declaration>& node) override;
	void generate() override;

	std::string toString() const override;

private:
	std::shared_ptr<TypeNode> m_typeNode;
	std::vector<std::unique_ptr<Declaration>> m_fields;
	std::vector<std::unique_ptr<Declaration>> m_methods;
};

extern std::shared_ptr<TypeNode> g_type;