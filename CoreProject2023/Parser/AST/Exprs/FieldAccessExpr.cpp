#include "FieldAccessExpr.h"
#include <Parser/Visitor/Visitor.h>
#include <Utils/ErrorManager.h>
#include <Module/Symbols/TypeNode.h>
#include <Module/LLVMGlobals.h>
#include <Module/LLVMUtils.h>

FieldAccessExpr::FieldAccessExpr(std::unique_ptr<Expression> expr, std::string memberName)
	: m_expr(std::move(expr)), m_memberName(std::move(memberName)) {

	// Getting the resulting type
	const std::shared_ptr<Type>& type = Type::dereference(m_expr->getType());
	if (isString(type->basicType)) {
		if (m_memberName == "data") {
			BasicType basicType = BasicType((u8)type->basicType - (u8)BasicType::STR8 + (u8)BasicType::C8);
			m_type = PointerType::createType(BasicType::POINTER, Type::createType(basicType), true);
		} else if (m_memberName == "size") {
			m_type = Type::createType(BasicType::U64, true);
		}
	} else if (type->basicType == BasicType::ARRAY) {
		if (m_memberName == "size") { // compile time
			m_type = Type::createType(BasicType::U64, true);
			return;
		}
	} else if (type->basicType == BasicType::DYN_ARRAY) {
		if (m_memberName == "data") {
			m_type = PointerType::createType(BasicType::POINTER, type->asPointerType()->elementType, true);
		} else if (m_memberName == "size") {
			m_type = Type::createType(BasicType::U64, true);
		}
	} else if (type->basicType == BasicType::TUPLE) {
		if (std::all_of(m_memberName.begin(), m_memberName.end(), isdigit)) {
			TupleType* tupType = type->asTupleType();
			if (size_t i = std::stoull(m_memberName); i < tupType->subTypes.size()) {
				m_type = tupType->subTypes[i]->copy(type->isConst);
			}
		}
	} else if (type->basicType == BasicType::TYPE_NODE) {
		TypeNode* typeNode = ((TypeNodeType*)type.get())->node.get();
		for (auto& var : typeNode->fields) {
			if (var.name == m_memberName) {
				m_type = var.type->copy(type->isConst);
			}
		}
	}

	if (m_type) {
		if (m_expr->isLVal()) {
			bool isConst = m_expr->getType()->isConst;
			m_type = PointerType::createType(BasicType::LVAL_REFERENCE, std::move(m_type));
		}

		return;
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
	const std::shared_ptr<Type>& type = Type::dereference(m_expr->getType());
	if (type->basicType == BasicType::ARRAY) {
		if (m_memberName == "size") { // compile time
			return llvm_utils::getConstantInt(((ArrayType*)type.get())->size, 64);
		}
	}

	llvm::Value* value = m_expr->generate();
	if (isTrueReference(m_expr->getType()->basicType)
		&& isTrueReference(m_expr->getType()->asPointerType()->elementType->basicType)) {
		value = g_builder->CreateLoad(m_expr->getType()->asPointerType()->elementType->to_llvm(), value);
	}

	if (isTrueReference(m_type->basicType)) {
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
	} else {
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
	}

	ASSERT(false, "Something went wrong");
	return nullptr;
}

std::string FieldAccessExpr::toString() const {
	return m_expr->toString() + "." + m_memberName;
}

bool FieldAccessExpr::isCompileTime() const {
	return m_expr->getType()->basicType == BasicType::ARRAY;
}
