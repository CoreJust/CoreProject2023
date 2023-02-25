#pragma once
#include "Declaration.h"
#include "../Exprs/Expression.h"
#include <Module/Symbols/Variable.h>

class VariableDeclaration : public Declaration {
	FRIEND_CLASS_VISITORS

public:
	VariableDeclaration(Variable* var, std::unique_ptr<Expression> value);

	void accept(Visitor* visitor, std::unique_ptr<Declaration>& node) override;
	void generate() override;

	std::string toString() const override;

private:
	Variable* m_variable;
	std::unique_ptr<Expression> m_value;
};