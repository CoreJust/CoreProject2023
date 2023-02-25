#pragma once
#include "Expression.h"
#include <Module/Symbols/Function.h>

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
	// Returns the string the corresponding token would have had
	static std::string conditionOpToString(ConditionOp op);

public:
	ConditionalExpr(
		std::vector<std::unique_ptr<Expression>> exprs,
		std::vector<ConditionOp> ops
	);

	void accept(Visitor* visitor, std::unique_ptr<Expression>& node) override;
	llvm::Value* generate() override;

	std::string toString() const override;

private:
	std::vector<std::unique_ptr<Expression>> m_exprs;
	std::vector<Function*> m_operatorFuncs;
	std::vector<ConditionOp> m_ops;
};