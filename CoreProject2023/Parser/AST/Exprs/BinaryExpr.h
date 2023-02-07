#pragma once
#include "Expression.h"
#include <Lexer/Token.h>

// all binary exprs apart from conditionals (==, !=, <, >, >=, <=) and special operators (is, as, in, ...)
class BinaryExpr final : public Expression {
	FRIEND_CLASS_VISITORS

public:
	BinaryExpr(std::unique_ptr<Expression> right, std::unique_ptr<Expression> left, TokenType op);

	void accept(Visitor* visitor, std::unique_ptr<Expression>& node) override;
	llvm::Value* generate() override;
	llvm::Value* generateRValue() override;

private:
	std::unique_ptr<Expression> m_right;
	std::unique_ptr<Expression> m_left;
	TokenType m_op;
};