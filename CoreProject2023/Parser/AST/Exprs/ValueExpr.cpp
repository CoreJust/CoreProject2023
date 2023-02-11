#include "ValueExpr.h"
#include <Parser/Visitor/Visitor.h>
#include <Utils/ErrorManager.h>
#include <Module/LLVMUtils.h>
#include <Module/LLVMGlobals.h>

ValueExpr::ValueExpr(Value val)
	: m_val(std::move(val)) {
	if (m_val.type == BasicType::POINTER) {
		m_type = std::make_unique<PointerType>(BasicType::POINTER, std::make_unique<Type>(BasicType::U8), true);
	} else {
		m_type = std::make_unique<Type>(m_val.type, true);
	}
}

void ValueExpr::accept(Visitor* visitor, std::unique_ptr<Expression>& node) {
	visitor->visit(this, node);
}

llvm::Value* ValueExpr::generate() {
	BasicType type = m_type->basicType;
	if (isInteger(type) || isChar(type) || type == BasicType::BOOL) {
		return llvm_utils::getConstantInt(m_val.value.uintVal, getBasicTypeSize(type), isSigned(type));
	} else if (isFloat(type)) {
		return llvm_utils::getConstantFP(m_val.value.floatVal, getBasicTypeSize(type));
	} else if (isString(type)) {
		u8 symbol_width = 8 * (int)pow(2, u8(type) - u8(BasicType::STR8));
		return llvm_utils::getConstantString(m_val.value.strVal, symbol_width);
	} else if (type == BasicType::POINTER) {
		return llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(g_context));
	}

	return nullptr;
}

llvm::Value* ValueExpr::generateRValue() {
	ErrorManager::parserError(
		ErrorID::E2103_NOT_A_REFERENCE, 
		m_errLine,
		"literal cannot be a reference"
	);

	return nullptr;
}

bool ValueExpr::isCompileTime() const {
	return true;
}
