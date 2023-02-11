#pragma once
#include "Expression.h"

class AssignmentExpr final : public Expression {
	FRIEND_CLASS_VISITORS

public:
	AssignmentExpr(
		std::unique_ptr<Expression> rval, 
		std::unique_ptr<Expression> expr
	);

	void accept(Visitor* visitor, std::unique_ptr<Expression>& node) override;
	llvm::Value* generate() override;
	llvm::Value* generateRValue() override;

private:
	std::unique_ptr<Expression> m_rval;
	std::unique_ptr<Expression> m_expr;
};