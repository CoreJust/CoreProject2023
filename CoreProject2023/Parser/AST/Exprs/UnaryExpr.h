#pragma once
#include "Expression.h"

class UnaryExpr final : public Expression {
	FRIEND_CLASS_VISITORS

public:
	enum UnaryOp : u8 {
		PLUS = 0,
		MINUS,

		POST_INC, // a++
		POST_DEC, // a--
		PRE_INC, // ++a
		PRE_DEC, // --a
		
		NOT, // ~a
		LOGICAL_NOT,

		ADRESS,
		DEREF,

		REF,
		REF_CONST,
		MOVE
	};

public:
	UnaryExpr(std::unique_ptr<Expression> expr, UnaryOp op);

	void accept(Visitor* visitor, std::unique_ptr<Expression>& node) override;
	llvm::Value* generate() override;
	llvm::Value* generateRValue() override;

private:
	std::unique_ptr<Expression> m_expr;
	UnaryOp m_op;
};