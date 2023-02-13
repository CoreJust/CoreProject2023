#pragma once
#include "Expression.h"

// fields
class FieldAccessExpr final : public Expression {
	FRIEND_CLASS_VISITORS

public:
	FieldAccessExpr(
		std::unique_ptr<Expression> expr,
		std::string memberName
	);

	void accept(Visitor* visitor, std::unique_ptr<Expression>& node) override;
	llvm::Value* generate() override;
	llvm::Value* generateRValue() override;

	bool isCompileTime() const override;

private:
	std::unique_ptr<Expression> m_expr;
	std::string m_memberName;
};