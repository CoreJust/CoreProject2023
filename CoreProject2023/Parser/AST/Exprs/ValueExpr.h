#pragma once
#include "Expression.h"
#include <Module/Value.h>

// Literals of integer, floating point, char, bool and string types as well as null
class ValueExpr final : public Expression {
	FRIEND_CLASS_VISITORS

public:
	ValueExpr(Value val);

	void accept(Visitor* visitor, std::unique_ptr<Expression>& node) override;
	llvm::Value* generate() override;
	llvm::Value* generateRValue() override;

private:
	Value m_val;
};