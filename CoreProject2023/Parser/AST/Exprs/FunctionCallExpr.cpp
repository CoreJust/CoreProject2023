#include "FunctionCallExpr.h"
#include <Parser/Visitor/Visitor.h>
#include <Utils/ErrorManager.h>
#include <Module/LLVMGlobals.h>
#include <Module/LLVMUtils.h>

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
		m_type = m_funcExpr->getType()->asFunctionType()->returnType->copy();
	}
}

void FunctionCallExpr::accept(Visitor* visitor, std::unique_ptr<Expression>& node) {
	visitor->visit(this, node);
}

llvm::Value* FunctionCallExpr::generate() {
	llvm::Function* funcVal = (llvm::Function*)m_funcExpr->generate();
	FunctionType* funcType = (FunctionType*)m_funcExpr->getType().get();

	return makeFunctionCall(funcVal, funcType, m_argExprs, m_errLine);
}

llvm::Value* FunctionCallExpr::makeFunctionCall(
	llvm::Function* functionValue,
	FunctionType* functionType,
	const std::vector<std::unique_ptr<Expression>>& args,
	u64 errLine
) {
	std::vector<llvm::Value*> argValues;
	for (size_t i = 0; i < args.size(); i++) {
		argValues.push_back(args[i]->generate());

		if (i < functionType->argTypes.size()) { // not va_args
			argValues.back() = llvm_utils::tryImplicitlyConvertTo(
				functionType->argTypes[i], // to type
				args[i]->getType(), // from type
				argValues.back(), // llvm value to be converted
				errLine, // the line the expression is at
				args[i]->isCompileTime() // is the expression available in compile time
			);
		} else {
			argValues.back() = llvm_utils::convertValueTo(
				Type::getTheVeryType(args[i]->getType()),
				args[i]->getType(),
				argValues.back()
			);
		}
	}

	llvm::Value* result = g_builder->CreateCall(
		functionType->to_llvmFunctionType(),
		functionValue,
		argValues
	);

	return result;
}

llvm::Value* FunctionCallExpr::makeFunctionCall(
	llvm::Function* functionValue,
	FunctionType* functionType,
	std::vector<llvm::Value*>& args,
	const std::vector<std::unique_ptr<Type>>& argTypes,
	const std::vector<bool>& isCompileTime,
	u64 errLine
) {
	for (size_t i = 0; i < args.size(); i++) {
		if (i < functionType->argTypes.size()) { // not va_args
			args.back() = llvm_utils::tryImplicitlyConvertTo(
				functionType->argTypes[i], // to type
				argTypes[i], // from type
				args.back(), // llvm value to be converted
				errLine, // the line the expression is at
				isCompileTime[i] // is the expression available in compile time
			);
		} else {
			args.back() = llvm_utils::convertValueTo(
				Type::getTheVeryType(argTypes[i]),
				argTypes[i],
				args.back()
			);
		}
	}

	llvm::Value* result = g_builder->CreateCall(
		functionType->to_llvmFunctionType(),
		functionValue,
		args
	);

	return result;
}
