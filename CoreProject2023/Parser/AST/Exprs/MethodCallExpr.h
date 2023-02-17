#pragma once
#include "Expression.h"
#include <Module/Symbols/Function.h>

class MethodCallExpr final : public Expression {
	FRIEND_CLASS_VISITORS

public:
	MethodCallExpr(
		Function* func,
		std::unique_ptr<Expression> thisExpr,
		std::vector<std::unique_ptr<Expression>> args
	);

	void accept(Visitor* visitor, std::unique_ptr<Expression>& node) override;
	llvm::Value* generate() override;
	llvm::Value* generateRValue() override;

private:
	Function* m_func;
	std::unique_ptr<Expression> m_thisExpr;
	std::vector<std::unique_ptr<Expression>> m_argExprs;
};
