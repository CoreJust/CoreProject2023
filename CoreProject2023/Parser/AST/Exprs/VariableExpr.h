#pragma once
#include "Expression.h"
#include <Module/Variable.h>

class VariableExpr final : public Expression {
	FRIEND_CLASS_VISITORS

public:
	VariableExpr(Variable* variable);

	void accept(Visitor* visitor, std::unique_ptr<Expression>& node) override;
	llvm::Value* generate() override;

private:
	Variable* m_variable;
};