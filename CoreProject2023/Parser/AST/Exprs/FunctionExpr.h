#pragma once
#include "Expression.h"
#include <Module/Symbols/Function.h>

class FunctionExpr final : public Expression {
	FRIEND_CLASS_VISITORS

public:
	FunctionExpr(Function* func);

	void accept(Visitor* visitor, std::unique_ptr<Expression>& node) override;
	llvm::Value* generate() override;

	std::string toString() const override;
	bool isCompileTime() const override;

private:
	Function* m_function;
};