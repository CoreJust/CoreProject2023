#include "MethodCallExpr.h"
#include <Parser/Visitor/Visitor.h>
#include <Utils/ErrorManager.h>
#include <Module/LLVMGlobals.h>
#include <Module/LLVMUtils.h>

MethodCallExpr::MethodCallExpr(
	Function* func,
	std::unique_ptr<Expression> thisExpr,
	std::vector<std::unique_ptr<Expression>> args)
	: m_func(func), m_thisExpr(std::move(thisExpr)), m_argExprs(std::move(args)) {
	ASSERT(m_func, "function cannot be no-null");
	ASSERT(m_thisExpr, "this expression cannot be no-null");

	m_type = m_func->prototype.getReturnType()->copy();
	if (isReference(m_type->basicType)) {
		m_isRVal = true;
	}
}

void MethodCallExpr::accept(Visitor* visitor, std::unique_ptr<Expression>& node) {
	visitor->visit(this, node);
}

llvm::Value* MethodCallExpr::generate() {
	std::unique_ptr<FunctionType> funcType = m_func->prototype.genType();

	std::vector<llvm::Value*> argValues;
	argValues.push_back(m_thisExpr->generateRValue());

	for (size_t i = 0; i < m_argExprs.size(); i++) {
		argValues.push_back(m_argExprs[i]->generate());

		if (i < funcType->argTypes.size()) { // not va_args
			argValues.back() = llvm_utils::tryImplicitlyConvertTo(
				funcType->argTypes[i + 1], // to type
				m_argExprs[i]->getType(), // from type
				argValues.back(), // llvm value to be converted
				m_errLine, // the line the expression is at
				m_argExprs[i]->isCompileTime() // is the expression available in compile time
			);
		}
	}

	/*
	for (auto& arg : argValues) {
		arg->print(llvm::errs());
		llvm::errs() << "\n";
	}

	m_func->functionValue->print(llvm::errs());
	llvm::errs() << "\n";
	*/

	llvm::Value* result = g_builder->CreateCall(
		funcType->to_llvmFunctionType(),
		m_func->functionValue,
		argValues
	);

	if (m_type->basicType == BasicType::REFERENCE) {
		llvm::Type* type = ((PointerType*)m_type.get())->elementType->to_llvm();
		result = g_builder->CreateLoad(type, result);
	}

	return result;
}

llvm::Value* MethodCallExpr::generateRValue() {
	if (m_type->basicType != BasicType::REFERENCE) {
		ErrorManager::parserError(
			ErrorID::E2103_NOT_A_REFERENCE,
			m_errLine,
			"function does not return a reference"
		);
	}

	std::unique_ptr<FunctionType> funcType = m_func->prototype.genType();

	std::vector<llvm::Value*> argValues;
	argValues.push_back(m_thisExpr->generate());

	for (size_t i = 0; i < m_argExprs.size(); i++) {
		argValues.push_back(m_argExprs[i]->generate());

		if (i < funcType->argTypes.size()) { // not va_args
			argValues.back() = llvm_utils::tryImplicitlyConvertTo(
				funcType->argTypes[i], // to type
				m_argExprs[i]->getType(), // from type
				argValues.back(), // llvm value to be converted
				m_errLine, // the line the expression is at
				m_argExprs[i]->isCompileTime() // is the expression available in compile time
			);
		}
	}

	return g_builder->CreateCall(
		(llvm::FunctionType*)funcType->to_llvm(),
		m_func->functionValue,
		argValues
	);
}
