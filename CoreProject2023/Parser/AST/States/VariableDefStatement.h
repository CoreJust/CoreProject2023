#pragma once
#include "Statement.h"
#include "../Exprs/Expression.h"
#include <Module/Symbols/Variable.h>

class VariableDefStatement final : public Statement {
	FRIEND_CLASS_VISITORS

public:
	VariableDefStatement(Variable var, std::unique_ptr<Expression> expr);

	void accept(Visitor* visitor, std::unique_ptr<Statement>& node) override;
	void generate() override;

private:
	Variable m_variable;
	std::unique_ptr<Expression> m_expr;
};