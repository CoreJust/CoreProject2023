#pragma once
#include "Expression.h"
#include <Module/Symbols/Function.h>

// Literals of integer, floating point, char, bool and string types as well as null
class TypeConversionExpr final : public Expression {
	FRIEND_CLASS_VISITORS

public:
	TypeConversionExpr(std::vector<std::unique_ptr<Expression>> args, std::shared_ptr<Type> type);

	void accept(Visitor* visitor, std::unique_ptr<Expression>& node) override;
	llvm::Value* generate() override;

	std::string toString() const override;

private:
	Function* chooseConstructor();

private:
	std::vector<std::unique_ptr<Expression>> m_args; // arguments of constructor/converted expression
	bool m_isConstructor = false;
};