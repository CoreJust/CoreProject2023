#pragma once
#include "Statement.h"
#include "../Exprs/Expression.h"
#include "VariableDefStatement.h"

// TODO: add for ranges
class ForStatement final : public Statement {
	FRIEND_CLASS_VISITORS

public:
	ForStatement(
		std::vector<std::unique_ptr<Statement>> varDefs,
		std::unique_ptr<Expression> condition,
		std::vector<std::unique_ptr<Expression>> increments,
		std::unique_ptr<Statement> body
	);

	void accept(Visitor* visitor, std::unique_ptr<Statement>& node) override;
	void generate() override;

	std::string toString() const override;

private:
	std::vector<std::unique_ptr<Statement>> m_varDefs;
	std::unique_ptr<Expression> m_condition;
	std::vector<std::unique_ptr<Expression>> m_increments;
	std::unique_ptr<Statement> m_body;
};