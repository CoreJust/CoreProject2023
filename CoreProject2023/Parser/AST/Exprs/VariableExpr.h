#pragma once
#include "Expression.h"
#include <Module/Symbols/Variable.h>

class VariableExpr final : public Expression {
	FRIEND_CLASS_VISITORS

public:
	VariableExpr(std::string moduleName, Variable* variable);
	VariableExpr(std::shared_ptr<TypeNode> typeNode, Variable* variable);
	~VariableExpr();

	void accept(Visitor* visitor, std::unique_ptr<Expression>& node) override;
	llvm::Value* generate() override;

private:
	bool m_isStaticTypeMember;
	union {
		std::string m_moduleName;
		std::shared_ptr<TypeNode> m_typeNode;
	};

	std::string m_name;
};