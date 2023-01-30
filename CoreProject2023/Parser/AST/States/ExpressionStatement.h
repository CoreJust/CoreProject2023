#pragma once
#include "Statement.h"
#include "../Exprs/Expression.h"

class ExpressionStatement final : public Statement {
	FRIEND_CLASS_VISITORS

public:
	ExpressionStatement(std::unique_ptr<Expression> expr);

	void accept(Visitor* visitor, std::unique_ptr<Statement>& node) override;
	void generate() override;

private:
	std::unique_ptr<Expression> m_expression;
};