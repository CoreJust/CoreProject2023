#pragma once
#include "Expression.h"

class TernaryExpr final : public Expression {
	FRIEND_CLASS_VISITORS

public:
	TernaryExpr(std::unique_ptr<Expression> condition, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right);

	void accept(Visitor* visitor, std::unique_ptr<Expression>& node) override;
	llvm::Value* generate() override;

	std::string toString() const override;

private:
	std::unique_ptr<Expression> m_condition;
	std::unique_ptr<Expression> m_left;
	std::unique_ptr<Expression> m_right;
};