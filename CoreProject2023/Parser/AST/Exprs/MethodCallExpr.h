#pragma once
#include "Expression.h"
#include <Module/Symbols/Function.h>

class MethodCallExpr final : public Expression {
	FRIEND_CLASS_VISITORS

public:
	MethodCallExpr(
		Function* func,
		std::vector<std::unique_ptr<Expression>> args
	);

	void accept(Visitor* visitor, std::unique_ptr<Expression>& node) override;
	llvm::Value* generate() override;

	std::string toString() const override;

private:
	Function* m_func;
	std::vector<std::unique_ptr<Expression>> m_argExprs;
};
