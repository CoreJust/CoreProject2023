#include "ArrayElementAccessExpr.h"
#include <Parser/Visitor/Visitor.h>
#include <Utils/ErrorManager.h>
#include <Module/Module.h>
#include <Module/LLVMUtils.h>
#include <Module/LLVMGlobals.h>
#include "FunctionCallExpr.h"

ArrayElementAccessExpr::ArrayElementAccessExpr(std::unique_ptr<Expression> arrayExpr, std::unique_ptr<Expression> indexExpr)
	: m_arrayExpr(std::move(arrayExpr)), m_indexExpr(std::move(indexExpr)) {
	if (isUserDefined(Type::getTheVeryType(m_arrayExpr->getType())->basicType)) {
		std::vector<std::unique_ptr<Type>> argTypes;
		argTypes.push_back(m_arrayExpr->getType()->copy());
		argTypes.push_back(m_indexExpr->getType()->copy());
		if (Function* operFunc = g_module->chooseOperator(
			"[",
			argTypes,
			{ m_arrayExpr->isCompileTime(), m_arrayExpr->isCompileTime() }
		)) {
			m_operatorFunc = operFunc;
			m_type = m_operatorFunc->prototype.getReturnType()->copy();

			return;
		}
	}

	const std::unique_ptr<Type>& arrayType = Type::getTheVeryType(m_arrayExpr->getType());
	if (!isString(arrayType->basicType) && arrayType->basicType != BasicType::ARRAY
		&& arrayType->basicType != BasicType::DYN_ARRAY && arrayType->basicType != BasicType::POINTER) {
		ErrorManager::parserError(
			ErrorID::E2008_INCORRECT_ARRAY_ELEMENT_ACCESS,
			m_errLine,
			"not an array type"
		);

		return;
	}

	if (!isImplicitlyConverible(m_indexExpr->getType(), std::make_unique<Type>(BasicType::U64)), m_indexExpr->isCompileTime()) {
		ErrorManager::parserError(
			ErrorID::E2008_INCORRECT_ARRAY_ELEMENT_ACCESS,
			m_errLine,
			"incorrect index type"
		);

		return;
	}

	if (arrayType->basicType == BasicType::ARRAY) {
		m_type = arrayType->asArrayType()->elementType->copy();
	} else if (isString(arrayType->basicType)) {
		m_type = std::make_unique<Type>(BasicType((u8)arrayType->basicType - (u8)BasicType::STR8 + (u8)BasicType::C8));
	} else { // pointer or dynamic array
		m_type = arrayType->asPointerType()->elementType->copy();
	}

	if (m_arrayExpr->isLVal()) {
		m_type = std::make_unique<PointerType>(BasicType::LVAL_REFERENCE, std::move(m_type), m_arrayExpr->getType()->isConst);
	}
}

void ArrayElementAccessExpr::accept(Visitor* visitor, std::unique_ptr<Expression>& node) {
	visitor->visit(this, node);
}

llvm::Value* ArrayElementAccessExpr::generate() {
	if (m_operatorFunc) {
		std::vector<std::unique_ptr<Expression>> args;
		args.push_back(std::move(m_arrayExpr));
		args.push_back(std::move(m_indexExpr));
		return FunctionCallExpr::makeFunctionCall(
			m_operatorFunc->functionValue,
			m_operatorFunc->prototype.genType().get(),
			args,
			m_errLine
		);
	}

	llvm::Value* arrayVal = m_arrayExpr->generate();
	const std::unique_ptr<Type>& arrayType = Type::getTheVeryType(m_arrayExpr->getType());
	arrayVal = llvm_utils::convertValueTo(arrayType, m_arrayExpr->getType(), arrayVal);

	if (isString(arrayType->basicType) || arrayType->basicType == BasicType::DYN_ARRAY) {
		arrayVal = g_builder->CreateGEP(
			Type::getTheVeryType(m_type)->to_llvm(),
			arrayVal,
			{ llvm_utils::getConstantInt(0, 32) }
		);
	}

	llvm::Value* indexVal = m_indexExpr->generate();
	indexVal = llvm_utils::convertValueTo(std::make_unique<Type>(BasicType::U64), m_indexExpr->getType(), indexVal);

	llvm::Value* elementVal = g_builder->CreateGEP(
		Type::getTheVeryType(m_type)->to_llvm(),
		arrayVal,
		{ indexVal }
	);

	if (isReference(m_type->basicType)) {
		return elementVal;
	} else {
		return g_builder->CreateLoad(Type::getTheVeryType(m_type)->to_llvm(), elementVal);
	}
}
