#pragma once
#include "Declaration.h"
#include "../Exprs/Expression.h"
#include <Module/Symbols/Variable.h>

class FieldDeclaration : public Declaration {
	FRIEND_CLASS_VISITORS

public:
	FieldDeclaration(std::shared_ptr<TypeNode> typeNode, Variable* field, std::unique_ptr<Expression> value);

	void accept(Visitor* visitor, std::unique_ptr<Declaration>& node) override;
	void generate() override;

	std::string toString() const override;

private:
	std::shared_ptr<TypeNode> m_typeNode;
	std::string m_name;
	bool m_isStatic;
	std::unique_ptr<Expression> m_value;
};