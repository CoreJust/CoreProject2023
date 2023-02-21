#pragma once
#include "Expression.h"
#include <Module/Symbols/Function.h>

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

		MOVE
	};

public:
	// Returns the string the corresponding token would have had
	static std::string unaryOpToString(UnaryOp op);
	static bool isUnaryOpDefinable(UnaryOp op);

public:
	UnaryExpr(std::unique_ptr<Expression> expr, UnaryOp op);

	void accept(Visitor* visitor, std::unique_ptr<Expression>& node) override;
	llvm::Value* generate() override;

private:
	llvm::Value* createIncOrDecrement(
		BasicType btype,
		bool isIncrement,
		bool isPostfix,
		bool isRVal
	);

private:
	std::unique_ptr<Expression> m_expr;
	Function* m_operatorFunc = nullptr; // in case the operator is defined as a function
	UnaryOp m_op;
};