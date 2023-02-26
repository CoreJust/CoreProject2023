#include "ValueExpr.h"
#include <Parser/Visitor/Visitor.h>
#include <Utils/ErrorManager.h>
#include <Module/LLVMUtils.h>
#include <Module/LLVMGlobals.h>

ValueExpr::ValueExpr(Value val)
	: m_val(std::move(val)) {
	if (m_val.type == BasicType::POINTER) {
		m_type = PointerType::createType(BasicType::POINTER, Type::createType(BasicType::U8), true);
	} else {
		m_type = Type::createType(m_val.type, true);
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

std::string ValueExpr::toString() const {
	auto makeCharPrintable = [](uint32_t ch, bool addQuots) -> std::string {
		std::string result = addQuots ? "'" : "";
		switch (ch) {
			case '\n': result += "\\n"; break;
			case '\0': result += "\\0"; break;
			case '\a': result += "\\a"; break;
			case '\t': result += "\\t"; break;
			case '\r': result += "\\r"; break;
			case '\v': result += "\\v"; break;
			case '\b': result += "\\b"; break;
			case '\f': result += "\\f"; break;
			case '\'': result += "\\'"; break;
			case '"': result += "\\\""; break;
			case '\\': result += "\\\\"; break;
		default: result += ch; break;
		}

		if (addQuots) {
			result += "'";
		}

		return result;
	};

	auto makeStringPrintable = [&](std::string str, int char_width) -> std::string {
		std::string result = "\"";
		for (size_t i = 0; i < str.size(); i += char_width) {
			switch (char_width) {
				case 1: str += makeCharPrintable(str[i], false); break;
				case 2: str += makeCharPrintable(*(uint16_t*)&str[i], false); break;
				case 4: str += makeCharPrintable(*(uint32_t*)&str[i], false); break;
			default: break;
			}
		}

		result += "\"";
		return result;
	};

	BasicType type = m_type->basicType;
	switch (type) {
	case BasicType::I8:
	case BasicType::I16:
	case BasicType::I32:
	case BasicType::I64:
		return std::to_string(m_val.value.intVal);
	case BasicType::U8:
	case BasicType::U16:
	case BasicType::U32:
	case BasicType::U64:
		return std::to_string(m_val.value.uintVal);
	case BasicType::F32:
	case BasicType::F64:
		return std::to_string(m_val.value.floatVal);
	case BasicType::BOOL:
		return m_val.value.uintVal ? "true" : "false";
	case BasicType::C8:
	case BasicType::C16:
	case BasicType::C32:
		return makeCharPrintable(m_val.value.uintVal, true);
	case BasicType::STR8:
		return makeStringPrintable(m_val.value.strVal, 1);
	case BasicType::STR16:
		return makeStringPrintable(m_val.value.strVal, 2);
	case BasicType::STR32:
		return makeStringPrintable(m_val.value.strVal, 4);
	default: return "";
	}
}

bool ValueExpr::isCompileTime() const {
	return true;
}
