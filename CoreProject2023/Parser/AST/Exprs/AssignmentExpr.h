#pragma once
#include "Expression.h"
#include <Module/Symbols/Function.h>

class AssignmentExpr final : public Expression {
	FRIEND_CLASS_VISITORS

public:
	enum AssignmentOp : u8 {
		EQUATE = 0,

		PLUS_EQ,
		MINUS_EQ,
		MULT_EQ,
		DIV_EQ,
		IDIV_EQ,
		MOD_EQ,
		POWER_EQ,

		AND_EQ,
		OR_EQ,
		XOR_EQ,
		LSHIFT_EQ,
		RSHIFT_EQ
	};

public:
	static std::string assignmentOpToString(AssignmentOp op);

public:
	AssignmentExpr(
		std::unique_ptr<Expression> lval, 
		std::unique_ptr<Expression> expr,
		AssignmentOp op
	);

	void accept(Visitor* visitor, std::unique_ptr<Expression>& node) override;
	llvm::Value* generate() override;

	std::string toString() const override;

private:
	std::unique_ptr<Expression> m_lval;
	std::unique_ptr<Expression> m_expr;
	Function* m_operatorFunc = nullptr;
	AssignmentOp m_op;
};