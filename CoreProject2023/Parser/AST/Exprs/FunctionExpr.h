#pragma once
#include "Expression.h"
#include <Module/Function.h>

class FunctionExpr final : public Expression {
	FRIEND_CLASS_VISITORS

public:
	FunctionExpr(Function* func);

	void accept(Visitor* visitor, std::unique_ptr<Expression>& node) override;
	llvm::Value* generate() override;
	llvm::Value* generateRValue() override;

	bool isCompileTime() const override;

private:
	Function* m_function;
};