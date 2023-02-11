#include "FunctionCallExpr.h"
#include <Parser/Visitor/Visitor.h>
#include <Utils/ErrorManager.h>
#include <Module/LLVMGlobals.h>

FunctionCallExpr::FunctionCallExpr(
	std::unique_ptr<Expression> func, 
	std::vector<std::unique_ptr<Expression>> args
) : 
	m_funcExpr(std::move(func)), 
	m_argExprs(std::move(args)) 
{
	if (m_funcExpr->getType()->basicType != BasicType::FUNCTION) {
		ErrorManager::parserError(
			ErrorID::E2102_CANNOT_BE_CALLED, 
			m_errLine, 
			""
		);
	} else {
		m_type = ((FunctionType*)m_funcExpr->getType().get())->returnType->copy();
		if (isReference(m_type->basicType)) {
			m_isRVal = true;
		}
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
#if 0
		argValues.back()->print(llvm::errs());
		llvm::errs() << "\n";
#endif
	}

#if 0
	funcVal->print(llvm::errs());
	llvm::errs() << "\n\n";
#endif

	llvm::Value* result = g_builder->CreateCall(
		funcType->to_llvmFunctionType(), 
		funcVal,
		argValues
	);

	if (m_type->basicType == BasicType::REFERENCE) {
		llvm::Type* type = ((PointerType*)m_type.get())->elementType->to_llvm();
		result = g_builder->CreateLoad(type, result);
	}

	return result;
}

llvm::Value* FunctionCallExpr::generateRValue() {
	if (m_type->basicType != BasicType::REFERENCE) {
		ErrorManager::parserError(
			ErrorID::E2103_NOT_A_REFERENCE, 
			m_errLine, 
			"function does not return a reference"
		);
	}

	llvm::Function* funcVal = (llvm::Function*)m_funcExpr->generate();
	FunctionType* funcType = (FunctionType*)m_funcExpr->getType().get();

	std::vector<llvm::Value*> argValues;
	for (auto& arg : m_argExprs) {
		argValues.push_back(arg->generate());
	}

	return g_builder->CreateCall(
		(llvm::FunctionType*)funcType->to_llvm(), 
		funcVal, 
		argValues
	);
}
