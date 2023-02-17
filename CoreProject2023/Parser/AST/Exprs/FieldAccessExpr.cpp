#include "FieldAccessExpr.h"
#include <Parser/Visitor/Visitor.h>
#include <Utils/ErrorManager.h>
#include <Module/Symbols/TypeNode.h>
#include <Module/LLVMGlobals.h>
#include <Module/LLVMUtils.h>

FieldAccessExpr::FieldAccessExpr(std::unique_ptr<Expression> expr, std::string memberName)
	: m_expr(std::move(expr)), m_memberName(std::move(memberName)) {
	m_isRVal = m_expr->isRVal();

	// Getting the resulting type
	const std::unique_ptr<Type>& type = Type::getTheVeryType(m_expr->getType());
	if (isString(type->basicType)) {
		if (m_memberName == "data") {
			BasicType basicType = BasicType((u8)type->basicType - (u8)BasicType::STR8 + (u8)BasicType::C8);
			m_type = std::make_unique<PointerType>(BasicType::POINTER, std::make_unique<Type>(basicType), true);
			return;
		} else if (m_memberName == "size") {
			m_type = std::make_unique<Type>(BasicType::U64, true);
			return;
		}
	} else if (type->basicType == BasicType::ARRAY) {
		if (m_memberName == "size") { // compile time
			m_type = std::make_unique<Type>(BasicType::U64, true);
			return;
		}
	} else if (type->basicType == BasicType::DYN_ARRAY) {
		if (m_memberName == "data") {
			m_type = type->copy();
			m_type->basicType = BasicType::POINTER;
			m_type->isConst = true;
			return;
		} else if (m_memberName == "size") {
			m_type = std::make_unique<Type>(BasicType::U64, true);
			return;
		}
	} else if (type->basicType == BasicType::TUPLE) {
		if (std::all_of(m_memberName.begin(), m_memberName.end(), isdigit)) {
			TupleType* tupType = (TupleType*)type.get();
			if (size_t i = std::stoull(m_memberName); i < tupType->subTypes.size()) {
				m_type = tupType->subTypes[i]->copy();
				m_type->isConst = type->isConst;
				return;
			}
		}
	} else if (type->basicType == BasicType::TYPE_NODE) {
		TypeNode* typeNode = ((TypeNodeType*)type.get())->node.get();
		for (auto& var : typeNode->fields) {
			if (var.name == m_memberName) {
				m_type = var.type->copy();
				m_type->isConst = type->isConst;
				return;
			}
		}
	}

	ErrorManager::parserError(
		ErrorID::E2006_NO_SUCH_MEMBER,
		m_errLine,
		"type " + type->toString() + " has no field " + m_memberName
	);
}

void FieldAccessExpr::accept(Visitor* visitor, std::unique_ptr<Expression>& node) {
	visitor->visit(this, node);
}

llvm::Value* FieldAccessExpr::generate() {
	const std::unique_ptr<Type>& type = Type::getTheVeryType(m_expr->getType());
	if (type->basicType == BasicType::ARRAY) {
		if (m_memberName == "size") { // compile time
			return llvm_utils::getConstantInt(((ArrayType*)type.get())->size, 64);
		}
	}

	llvm::Value* value = m_expr->generate();
	if (isString(type->basicType)) {
		if (m_memberName == "data") {
			return g_builder->CreateExtractValue(value, llvm::ArrayRef<u32>(0));
		} else if (m_memberName == "size") {
			return g_builder->CreateExtractValue(value, llvm::ArrayRef<u32>(1));
		}
	} else if (type->basicType == BasicType::DYN_ARRAY) {
		if (m_memberName == "data") {
			return g_builder->CreateExtractValue(value, llvm::ArrayRef<u32>(0));
		} else if (m_memberName == "size") {
			return g_builder->CreateExtractValue(value, llvm::ArrayRef<u32>(1));
		}
	} else if (type->basicType == BasicType::TUPLE) {
		if (std::all_of(m_memberName.begin(), m_memberName.end(), isdigit)) {
			TupleType* tupType = (TupleType*)type.get();
			if (size_t i = std::stoull(m_memberName); i < tupType->subTypes.size()) {
				return g_builder->CreateExtractValue(value, llvm::ArrayRef<u32>(i));
			}
		}
	} else if (type->basicType == BasicType::TYPE_NODE) {
		TypeNode* typeNode = ((TypeNodeType*)type.get())->node.get();
		for (size_t i = 0; i < typeNode->fields.size(); i++) {
			if (typeNode->fields[i].name == m_memberName) {
				return g_builder->CreateExtractValue(value, llvm::ArrayRef<u32>(i));
			}
		}
	}

	ASSERT(false, "Something went wrong");
	return nullptr;
}

llvm::Value* FieldAccessExpr::generateRValue() {
	if (!m_isRVal) {
		ASSERT(false, "Something went wrong");
		return nullptr;
	}

	const std::unique_ptr<Type>& type = Type::getTheVeryType(m_expr->getType());
	llvm::Value* value = m_expr->generateRValue();
	llvm::Constant* zeroInt = llvm_utils::getConstantInt(0, 32);
	if (isString(type->basicType)) {
		if (m_memberName == "data") {
			return g_builder->CreateGEP(type->to_llvm(), value, { zeroInt, llvm_utils::getConstantInt(0, 32) });
		} else if (m_memberName == "size") {
			return g_builder->CreateGEP(type->to_llvm(), value, { zeroInt, llvm_utils::getConstantInt(1, 32) });
		}
	} else if (type->basicType == BasicType::DYN_ARRAY) {
		if (m_memberName == "data") {
			return g_builder->CreateGEP(type->to_llvm(), value, { zeroInt, llvm_utils::getConstantInt(0, 32) });
		} else if (m_memberName == "size") {
			return g_builder->CreateGEP(type->to_llvm(), value, { zeroInt, llvm_utils::getConstantInt(1, 32) });
		}
	} else if (type->basicType == BasicType::TUPLE) {
		if (std::all_of(m_memberName.begin(), m_memberName.end(), isdigit)) {
			TupleType* tupType = (TupleType*)type.get();
			if (size_t i = std::stoull(m_memberName); i < tupType->subTypes.size()) {
				return g_builder->CreateGEP(type->to_llvm(), value, { zeroInt, llvm_utils::getConstantInt(i, 32) });
			}
		}
	} else if (type->basicType == BasicType::TYPE_NODE) {
		TypeNode* typeNode = ((TypeNodeType*)type.get())->node.get();
		for (size_t i = 0; i < typeNode->fields.size(); i++) {
			if (typeNode->fields[i].name == m_memberName) {
				return g_builder->CreateGEP(type->to_llvm(), value, { zeroInt, llvm_utils::getConstantInt(i, 32) });
			}
		}
	}

	ASSERT(false, "Something went wrong");
	return nullptr;
}

bool FieldAccessExpr::isCompileTime() const {
	return m_expr->getType()->basicType == BasicType::ARRAY;
}
