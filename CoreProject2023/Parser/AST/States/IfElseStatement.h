#pragma once
#include "Statement.h"
#include "../Exprs/Expression.h"

class IfElseStatement final : public Statement {
	FRIEND_CLASS_VISITORS

public:
	IfElseStatement(
		std::vector<std::unique_ptr<Statement>> bodies,
		std::vector<std::unique_ptr<Expression>> conditions
	);

	void accept(Visitor* visitor, std::unique_ptr<Statement>& node) override;
	void generate() override;

private:
	// Must contain at least one; if there is one extra body - it is the else statement
	std::vector<std::unique_ptr<Expression>> m_conditions;
	std::vector<std::unique_ptr<Statement>> m_bodies;
};