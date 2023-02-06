#include "FunctionCallExpr.h"
#include <Parser/Visitor/Visitor.h>
#include <Utils/ErrorManager.h>

FunctionCallExpr::FunctionCallExpr(std::unique_ptr<Expression> func, std::vector<std::unique_ptr<Expression>> args)
	: m_funcExpr(std::move(func)), m_argExprs(std::move(args)) {
	if (m_funcExpr->getType()->basicType != BasicType::FUNCTION) {
		ErrorManager::parserError(ErrorID::E2102_CANNOT_BE_CALLED, m_errLine, "");
	} else {
		m_type = ((FunctionType*)m_funcExpr->getType().get())->returnType->copy();
	}
}

void FunctionCallExpr::accept(Visitor* visitor, std::unique_ptr<Expression>& node) {
	visitor->visit(this, node);
}

llvm::Value* FunctionCallExpr::generate() {
	llvm::Function* funcVal = (llvm::Function*)m_funcExpr->generate();
	FunctionType* funcType = (FunctionType*)m_funcExpr->getType().get();

	std::vector<llvm::Value*> argValues;
	for (auto& arg : m_argExprs) {
		argValues.push_back(arg->generate());
	}

	return g_builder->CreateCall((llvm::FunctionType*)funcType->to_llvm(), funcVal, argValues);
}
