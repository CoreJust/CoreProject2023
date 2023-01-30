#pragma once
#include "Expression.h"

class ValueExpr final : public Expression {
	FRIEND_CLASS_VISITORS

public:
	ValueExpr(int val);

	void accept(Visitor* visitor, std::unique_ptr<Expression>& node) override;
	llvm::Value* generate() override;

private:
	int m_val;
};