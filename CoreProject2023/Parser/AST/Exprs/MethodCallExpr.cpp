#include "MethodCallExpr.h"
#include <Parser/Visitor/Visitor.h>
#include <Utils/ErrorManager.h>
#include <Module/LLVMGlobals.h>
#include <Module/LLVMUtils.h>
#include "FunctionCallExpr.h"

MethodCallExpr::MethodCallExpr(
	Function* func,
	std::vector<std::unique_ptr<Expression>> args)
	: m_func(func), m_argExprs(std::move(args)) {
	ASSERT(m_func, "function cannot be no-null");
	ASSERT(m_argExprs.size(), "this expression cannot be no-null");

	m_type = m_func->prototype.getReturnType()->copy();
}

void MethodCallExpr::accept(Visitor* visitor, std::unique_ptr<Expression>& node) {
	visitor->visit(this, node);
}

llvm::Value* MethodCallExpr::generate() {
	std::unique_ptr<FunctionType> funcType = m_func->prototype.genType();

	return FunctionCallExpr::makeFunctionCall(
		m_func->getValue(),
		funcType.get(),
		m_argExprs,
		m_errLine
	);
}
