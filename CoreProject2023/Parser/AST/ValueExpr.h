#pragma once
#include "Expression.h"

class ValueExpr final : public Expression {
public:
	int m_val;

public:
	ValueExpr(int val);

	void accept(Visitor* visitor, std::unique_ptr<Expression>& node) override;
	llvm::Value* generate() override;
};