#pragma once
#include "Expression.h"
#include <Module/Symbols/Function.h>

// all binary exprs apart from conditionals (==, !=, <, >, >=, <=) and special operators (is, as, in, ...)
class BinaryExpr final : public Expression {
	FRIEND_CLASS_VISITORS
	
public:
	enum BinaryOp : u8 {
		PLUS = 0,
		MINUS,
		MULT,
		DIV,
		IDIV,
		MOD,
		POWER,

		AND,
		OR,
		XOR,
		LSHIFT,
		RSHIFT,

		LOGICAL_AND,
		LOGICAL_OR
	};

public:
	// Returns the string the corresponding token would have had
	static std::string binaryOpToString(BinaryOp op);
	static bool isBinaryOpDefinable(BinaryOp op);

public:
	BinaryExpr(
		std::unique_ptr<Expression> left,
		std::unique_ptr<Expression> right,
		BinaryOp op
	);

	void accept(Visitor* visitor, std::unique_ptr<Expression>& node) override;
	llvm::Value* generate() override;

public:
	static llvm::Value* generateBinaryOperation(
		std::unique_ptr<Expression>& left,
		std::unique_ptr<Expression>& right,
		std::unique_ptr<Type>& resultingType,
		BinaryOp op,
		bool convertToResultingType
	);

private:
	std::unique_ptr<Expression> m_right;
	std::unique_ptr<Expression> m_left;
	Function* m_operatorFunc = nullptr; // in case the operator is defined as a function
	BinaryOp m_op;
};