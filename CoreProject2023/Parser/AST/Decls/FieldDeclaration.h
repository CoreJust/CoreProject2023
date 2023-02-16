#pragma once
#include "Declaration.h"
#include "../Exprs/Expression.h"
#include <Module/Symbols/Variable.h>

class FieldDeclaration : public Declaration {
	FRIEND_CLASS_VISITORS

public:
	FieldDeclaration(Variable* field, std::unique_ptr<Expression> value);

	void accept(Visitor* visitor, std::unique_ptr<Declaration>& node) override;
	void generate() override;

private:
	Variable* m_field;
	std::unique_ptr<Expression> m_value;
};