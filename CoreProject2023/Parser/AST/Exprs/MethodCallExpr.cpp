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

	m_type = m_func->prototype.getReturnType();
	m_safety = m_func->prototype.getQualities().getSafety();
	g_safety.tryUse(m_safety, m_errLine);
}

void MethodCallExpr::accept(Visitor* visitor, std::unique_ptr<Expression>& node) {
	visitor->visit(this, node);
}

llvm::Value* MethodCallExpr::generate() {
	std::shared_ptr<FunctionType> funcType = m_func->prototype.genType();

	return FunctionCallExpr::makeFunctionCall(
		m_func->getValue(),
		funcType.get(),
		m_argExprs,
		m_errLine
	);
}

std::string MethodCallExpr::toString() const {
	std::string result =
		m_argExprs[0]->toString()
		+ "."
		+ m_func->prototype.toString()
		+ "(";

	for (size_t i = 1; i < m_argExprs.size(); i++) {
		result += m_argExprs[i]->toString();
		result += ", ";
	}
	
	result.pop_back();
	result.pop_back();

	result += ')';

	return result;
}
