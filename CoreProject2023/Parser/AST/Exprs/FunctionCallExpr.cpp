#include "FunctionCallExpr.h"
#include <Parser/Visitor/Visitor.h>

FunctionCallExpr::FunctionCallExpr(std::unique_ptr<Expression> func, std::vector<std::unique_ptr<Expression>> args)
	: m_funcExpr(std::move(func)), m_argExprs(std::move(args)) {

}

void FunctionCallExpr::accept(Visitor* visitor, std::unique_ptr<Expression>& node) {
	visitor->visit(this, node);
}

llvm::Value* FunctionCallExpr::generate() {
	llvm::Function* funcVal = (llvm::Function*)m_funcExpr->generate();

	std::vector<llvm::Value*> argValues;
	for (auto& arg : m_argExprs) {
		argValues.push_back(arg->generate());
	}

	llvm::FunctionType* funcType = funcVal->getFunctionType();
	return g_builder->CreateCall(funcType, funcVal, argValues);
}
