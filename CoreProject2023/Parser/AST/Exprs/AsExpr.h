#pragma once
#include "Expression.h"

// Conversion bit-to-bit
class AsExpr final : public Expression {
	FRIEND_CLASS_VISITORS

public:
	AsExpr(std::unique_ptr<Expression> arg, std::shared_ptr<Type> type);

	void accept(Visitor* visitor, std::unique_ptr<Expression>& node) override;
	llvm::Value* generate() override;

	std::string toString() const override;

private:
	std::unique_ptr<Expression> m_arg; // arguments of constructor/converted expression
};