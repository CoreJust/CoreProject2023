#pragma once
#include "Statement.h"
#include "../Exprs/Expression.h"

class WhileStatement final : public Statement {
	FRIEND_CLASS_VISITORS

public:
	WhileStatement(std::unique_ptr<Expression> condition, std::unique_ptr<Statement> body);

	void accept(Visitor* visitor, std::unique_ptr<Statement>& node) override;
	void generate() override;

	std::string toString() const override;

private:
	std::unique_ptr<Expression> m_condition;
	std::unique_ptr<Statement> m_body;
};