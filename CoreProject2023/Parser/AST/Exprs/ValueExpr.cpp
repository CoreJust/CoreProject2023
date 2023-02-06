#include "ValueExpr.h"
#include <Parser/Visitor/Visitor.h>
#include <Module/LLVMUtils.h>

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
		return llvm::ConstantInt::get(g_context, llvm::APInt(getBasicTypeSize(type), m_val.value.uintVal, isSigned(type)));
	} else if (isFloat(type)) {
		if (type == BasicType::F32) {
			return llvm::ConstantFP::get(g_context, llvm::APFloat((float)m_val.value.floatVal));
		} else {
			return llvm::ConstantFP::get(g_context, llvm::APFloat(m_val.value.floatVal));
		}
	} else if (isString(type)) {
		if (type == BasicType::STR8) {
			return llvm::ConstantStruct::get((llvm::StructType*)m_type->to_llvm(),
				llvm::ArrayRef<llvm::Constant*>({
					g_builder->CreateGlobalString(m_val.value.strVal), llvm_utils::getConstantInt(0, 64, false)
				}));
		} else if (type == BasicType::STR16) {
			std::vector<llvm::Constant*> buffer;
			std::string& str = m_val.value.strVal;
			for (size_t i = 0; i < str.size(); i += 2) {
				buffer.push_back(llvm::ConstantInt::get(g_context, llvm::APInt(16, *(i16*)&str[i], true)));
			}

			buffer.push_back(llvm::ConstantInt::get(g_context, llvm::APInt(16, 0, true))); // terminating zero
			llvm::Constant* value = llvm::ConstantArray::get(
				llvm::ArrayType::get(llvm::Type::getInt16Ty(g_context), str.size() / 2 + 1),
				buffer
			);

			return llvm::ConstantStruct::get(
					(llvm::StructType*)m_type->to_llvm(),
					llvm::ArrayRef<llvm::Constant*>({ value, llvm_utils::getConstantInt(0, 64, false) })
				);
		} else if (type == BasicType::STR32) {
			std::vector<llvm::Constant*> buffer;
			std::string& str = m_val.value.strVal;
			for (size_t i = 0; i < str.size(); i += 4) {
				buffer.push_back(llvm::ConstantInt::get(g_context, llvm::APInt(32, *(i32*)&str[i], true)));
			}

			buffer.push_back(llvm::ConstantInt::get(g_context, llvm::APInt(32, 0, true))); // terminating zero
			llvm::Constant* value = llvm::ConstantArray::get(
				llvm::ArrayType::get(llvm::Type::getInt32Ty(g_context), str.size() / 4 + 1),
				buffer
			);

			return llvm::ConstantStruct::get(
				(llvm::StructType*)m_type->to_llvm(),
				llvm::ArrayRef<llvm::Constant*>({ value, llvm_utils::getConstantInt(0, 64, false) })
			);
		}
	} else if (type == BasicType::POINTER) {
		return llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(g_context));
	}

	return nullptr;
}
