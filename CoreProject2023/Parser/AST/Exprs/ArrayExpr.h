#pragma once
#include <vector>
#include "Expression.h"

// Literals of integer, floating point, char, bool and string types as well as null
class ArrayExpr final : public Expression {
	FRIEND_CLASS_VISITORS

public:
	ArrayExpr(std::shared_ptr<Type> elemType, u64 size, std::vector<std::unique_ptr<Expression>> values);

	void accept(Visitor* visitor, std::unique_ptr<Expression>& node) override;
	llvm::Value* generate() override;

	std::string toString() const override;
	bool isCompileTime() const override;

private:
	std::vector<std::unique_ptr<Expression>> m_values;
};