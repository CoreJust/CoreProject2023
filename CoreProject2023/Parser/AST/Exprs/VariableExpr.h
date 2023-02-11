#pragma once
#include "Expression.h"
#include <Module/Variable.h>

class VariableExpr final : public Expression {
	FRIEND_CLASS_VISITORS

public:
	VariableExpr(std::string moduleName, Variable* variable);

	void accept(Visitor* visitor, std::unique_ptr<Expression>& node) override;
	llvm::Value* generate() override;
	llvm::Value* generateRValue() override;

private:
	std::string m_moduleName;
	std::string m_name;
};