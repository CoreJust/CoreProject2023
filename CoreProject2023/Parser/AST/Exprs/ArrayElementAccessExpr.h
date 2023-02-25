#pragma once
#include "Expression.h"
#include <Module/Symbols/Function.h>

class ArrayElementAccessExpr final : public Expression {
	FRIEND_CLASS_VISITORS

public:
	ArrayElementAccessExpr(
		std::unique_ptr<Expression> arrayExpr,
		std::unique_ptr<Expression> indexExpr
	);

	void accept(Visitor* visitor, std::unique_ptr<Expression>& node) override;
	llvm::Value* generate() override;

	std::string toString() const override;

private:
	std::unique_ptr<Expression> m_arrayExpr;
	std::unique_ptr<Expression> m_indexExpr;
	Function* m_operatorFunc = nullptr;
};