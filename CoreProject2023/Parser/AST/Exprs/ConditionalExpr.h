#pragma once
#include "Expression.h"

// ==, !=, <, >, >=, <=
class ConditionalExpr final : public Expression {
	FRIEND_CLASS_VISITORS

public:
	enum ConditionOp : u8 {
		EQUALS = 0,
		NOT_EQUALS,
		LESS,
		GREATER,
		LESS_OR_EQUAL,
		GREATER_OR_EQUAL
	};

public:
	ConditionalExpr(
		std::vector<std::unique_ptr<Expression>> exprs,
		std::vector<ConditionOp> ops
	);

	void accept(Visitor* visitor, std::unique_ptr<Expression>& node) override;
	llvm::Value* generate() override;
	llvm::Value* generateRValue() override;

private:
	std::vector<std::unique_ptr<Expression>> m_exprs;
	std::vector<ConditionOp> m_ops;
};