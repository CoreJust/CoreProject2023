#pragma once
#include "Expression.h"
#include <Module/Symbols/Function.h>

class FunctionCallExpr final : public Expression {
	FRIEND_CLASS_VISITORS

public:
	FunctionCallExpr(
		std::unique_ptr<Expression> func, 
		std::vector<std::unique_ptr<Expression>> args
	);

	void accept(Visitor* visitor, std::unique_ptr<Expression>& node) override;
	llvm::Value* generate() override;

public:
	static llvm::Value* makeFunctionCall(
		llvm::Function* functionValue,
		FunctionType* functionType,
		const std::vector<std::unique_ptr<Expression>>& args,
		u64 errLine
	);

	static llvm::Value* makeFunctionCall(
		llvm::Function* functionValue,
		FunctionType* functionType,
		std::vector<llvm::Value*>& args,
		const std::vector<std::shared_ptr<Type>>& argTypes,
		const std::vector<bool>& isCompileTime,
		u64 errLine
	);

private:
	std::unique_ptr<Expression> m_funcExpr;
	std::vector<std::unique_ptr<Expression>> m_argExprs;
};
