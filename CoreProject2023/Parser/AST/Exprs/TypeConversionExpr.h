#pragma once
#include "Expression.h"

// Literals of integer, floating point, char, bool and string types as well as null
class TypeConversionExpr final : public Expression {
	FRIEND_CLASS_VISITORS

public:
	TypeConversionExpr(std::unique_ptr<Expression> expr, std::unique_ptr<Type> type);

	void accept(Visitor* visitor, std::unique_ptr<Expression>& node) override;
	llvm::Value* generate() override;

private:
	std::unique_ptr<Expression> m_expr;
};