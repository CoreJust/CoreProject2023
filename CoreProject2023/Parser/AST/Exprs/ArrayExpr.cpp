#include "ArrayExpr.h"
#include <llvm/IR/Constants.h>
#include <Parser/Visitor/Visitor.h>
#include <Utils/ErrorManager.h>
#include <Module/LLVMUtils.h>
#include <Module/LLVMGlobals.h>

ArrayExpr::ArrayExpr(std::unique_ptr<Type> elemType, u64 size, std::vector<std::unique_ptr<Expression>> values)
	: m_values(std::move(values)) {
	if (elemType) {
		if (size == 0) {
			size = m_values.size();
		}

		m_type = std::make_unique<ArrayType>(std::move(elemType), size, true);

		if (m_values.size() > size) {
			ErrorManager::parserError(
				ErrorID::E2009_TOO_MANY_ARRAY_ELEMENTS,
				m_errLine,
				"there are " + std::to_string(m_values.size()) + " elements, while only " + std::to_string(size) + " were declared"
			);
		}
	} else {
		size = m_values.size();
		m_type = std::make_unique<ArrayType>(m_values[0]->getType()->copy(), size, true);
	}
}

void ArrayExpr::accept(Visitor* visitor, std::unique_ptr<Expression>& node) {
	visitor->visit(this, node);
}

llvm::Value* ArrayExpr::generate() {
	const std::unique_ptr<Type>& elemType = m_type->asArrayType()->elementType;
	std::vector<llvm::Constant*> initialValues;

	for (auto& val : m_values) {
		if (val->isCompileTime()) {
			initialValues.push_back((llvm::Constant*)val->generate());
			initialValues.back() = (llvm::Constant*)llvm_utils::tryImplicitlyConvertTo(
				elemType,
				val->getType(),
				initialValues.back(),
				m_errLine,
				true
			);
		} else {
			initialValues.push_back(llvm_utils::getDefaultValueOf(elemType));
		}
	}

	if (u64 size = m_type->asArrayType()->size; size > m_values.size()) {
		while (size-- > m_values.size()) {
			initialValues.push_back(llvm_utils::getDefaultValueOf(elemType));
		}
	}

	llvm::ArrayType* llvmArrType = m_type->asArrayType()->to_llvmArrayType();
	llvm::Constant* arr = llvm::ConstantArray::get(llvmArrType, initialValues);
	llvm::Value* alloc = llvm_utils::createGlobalValue(llvmArrType, arr);

	llvm::Type* llvmElemType = elemType->to_llvm();
	llvm::Constant* zero = llvm_utils::getConstantInt(0, 32);
	u64 i = 0;
	for (auto& val : m_values) {
		if (!val->isCompileTime()) {
			llvm::Value* llvmValue = val->generate();
			llvmValue = llvm_utils::tryImplicitlyConvertTo(
				elemType,
				val->getType(),
				llvmValue,
				m_errLine,
				false
			);

			llvm::Value* elemPtr = g_builder->CreateGEP(
				llvmArrType,
				alloc,
				{ zero, llvm_utils::getConstantInt(i++, 32) }
			);

			g_builder->CreateStore(llvmValue, elemPtr);
		}
	}

	return alloc;
}

bool ArrayExpr::isCompileTime() const {
	for (auto& val : m_values) {
		if (!val->isCompileTime()) {
			return false;
		}
	}

	return true;
}
