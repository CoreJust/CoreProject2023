#pragma once
#include "Expression.h"

class FunctionCallExpr final : public Expression {
	FRIEND_CLASS_VISITORS

public:
	FunctionCallExpr(std::unique_ptr<Expression> func, std::vector<std::unique_ptr<Expression>> args);

	void accept(Visitor* visitor, std::unique_ptr<Expression>& node) override;
	llvm::Value* generate() override;

private:
	std::unique_ptr<Expression> m_funcExpr;
	std::vector<std::unique_ptr<Expression>> m_argExprs;
};
