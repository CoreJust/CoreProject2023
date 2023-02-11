#include "UnaryExpr.h"
#include <Parser/Visitor/Visitor.h>
#include <Utils/ErrorManager.h>
#include <Module/LLVMUtils.h>
#include <Module/LLVMGlobals.h>

UnaryExpr::UnaryExpr(std::unique_ptr<Expression> expr, UnaryOp op)
	: m_expr(std::move(expr)), m_op(op) {
	switch (m_op) {
		case UnaryExpr::PLUS:
		case UnaryExpr::NOT:
			m_type = m_expr->getType()->copy();
			break;
		case UnaryExpr::MINUS:
			m_type = m_expr->getType()->copy();
			if (isUnsigned(m_type->basicType)) {
				ErrorManager::typeError(
					ErrorID::E3054_CANNOT_NEGATE_UNSIGNED_INT,
					m_errLine, 
					"tried to negate " + m_type->toString()
				);
			}

			break;
		case UnaryExpr::PRE_INC:
		case UnaryExpr::PRE_DEC:
			m_isRVal = true;
		case UnaryExpr::POST_INC:
		case UnaryExpr::POST_DEC:
			if (!m_expr->isRVal()) {
				ErrorManager::typeError(
					ErrorID::E3056_MUST_BE_A_REFERENCE,
					m_errLine,
					"increment/decrement of " + m_type->toString()
				);
			}

			if (isReference(m_expr->getType()->basicType)) {
				m_type = ((PointerType*)m_expr->getType().get())->elementType->copy();
			} else {
				m_type = m_expr->getType()->copy();
			}

			break;
		case UnaryExpr::LOGICAL_NOT:
			m_type = std::make_unique<Type>(BasicType::BOOL);
			break;
		case UnaryExpr::ADRESS:
			m_type = std::make_unique<PointerType>(BasicType::POINTER, m_expr->getType()->copy());
			break;
		case UnaryExpr::DEREF:
			if (auto btype = m_expr->getType()->basicType;
				btype != BasicType::POINTER
				&& btype != BasicType::OPTIONAL
				&& btype != BasicType::ARRAY
				&& !isUserDefined(btype)) 
			{
				ErrorManager::typeError(
					ErrorID::E3053_ONLY_POINTERS_CAN_BE_DEREFERENCED, 
					m_errLine,
					"tried to derefence" + m_expr->getType()->toString()
				);
			} else {
				m_isRVal = true;
				if (m_expr->getType()->basicType == BasicType::ARRAY) {
					m_type = ((ArrayType*)m_expr->getType().get())->elementType->copy();
				} else {
					m_type = ((PointerType*)m_expr->getType().get())->elementType->copy();
				}
			}

			break;
		case UnaryExpr::REF:
			m_type = std::make_unique<PointerType>(BasicType::REFERENCE, m_expr->getType()->copy());
			m_isRVal = true;
			if (!m_expr->isRVal()) {
				ErrorManager::typeError(
					ErrorID::E3055_CANNOT_GET_REFERENCE, 
					m_errLine,
					"trying to get reference of " + m_expr->getType()->toString()
				);
			} else if (m_expr->getType()->isConst) {
				ErrorManager::typeError(
					ErrorID::E3057_IS_A_CONSTANT, 
					m_errLine,
					"cannot get a non-const reference from a constant"
				);
			}

			break;
		case UnaryExpr::REF_CONST:
			m_type = std::make_unique<PointerType>(BasicType::REFERENCE, m_expr->getType()->copy(), true);
			m_isRVal = true;
			break;
		case UnaryExpr::MOVE:
			m_type = std::make_unique<PointerType>(BasicType::RVAL_REFERENCE, m_expr->getType()->copy());
			m_isRVal = true;
			break;
	default:
		ASSERT(false, "wrong operator");
		break;
	}

	if (!m_isRVal && isReference(m_type->basicType)) {
		PointerType* ptrType = (PointerType*)m_type.get();
		std::unique_ptr<Type> tmp = ptrType->elementType->copy();
		m_type = std::move(tmp);
	}
}

void UnaryExpr::accept(Visitor* visitor, std::unique_ptr<Expression>& node) {
	visitor->visit(this, node);
}

llvm::Value* UnaryExpr::generate() {
	// Operations for which expression type is not important
	if (m_op == UnaryOp::ADRESS || m_op == UnaryOp::REF) {
		return m_expr->generateRValue();
	} else if (m_op == UnaryOp::DEREF) {
		llvm::Value* value = m_expr->generate();
		if (m_expr->getType()->basicType == BasicType::OPTIONAL) {
			return g_builder->CreateExtractValue(value, { 0 });
		}

		return g_builder->CreateLoad(m_type->to_llvm(), value);
	} else if (m_op == UnaryOp::REF_CONST) {
		if (m_expr->isRVal()) {
			return m_expr->generateRValue();
		} else {
			llvm::Value* alloc = llvm_utils::createLocalVariable(
				g_function->functionValue, 
				m_expr->getType(), 
				"$ref"
			);

			g_builder->CreateStore(m_expr->generate(), alloc);
			return alloc;
		}
	} else if (m_op == UnaryOp::MOVE) {
		llvm::Value* value = m_expr->generateRValue();
		const std::unique_ptr<Type>& type = isPointer(m_expr->getType()->basicType) ?
			((PointerType*)m_expr->getType().get())->elementType
			: m_expr->getType();

		llvm::Value* result = g_builder->CreateLoad(type->to_llvm(), value);
		g_builder->CreateStore(llvm_utils::getDefaultValueOf(type), value);
		return result;
	} else if (m_op == UnaryOp::LOGICAL_NOT) {
		llvm::Value* value = m_expr->generate();
		value = llvm_utils::convertToBool(m_expr->getType(), value);
		return g_builder->CreateNot(value);
	}
	
	BasicType btype = m_type->basicType;
	if (isInteger(btype) || btype == BasicType::BOOL) {
		switch (m_op) {
			case UnaryExpr::PLUS: return m_expr->generate();
			case UnaryExpr::MINUS: return g_builder->CreateNeg(m_expr->generate());
			case UnaryExpr::POST_INC: return createIncOrDecrement(btype, true, true, false);
			case UnaryExpr::POST_DEC: return createIncOrDecrement(btype, false, true, false);
			case UnaryExpr::PRE_INC: return createIncOrDecrement(btype, true, false, false);
			case UnaryExpr::PRE_DEC: return createIncOrDecrement(btype, false, false, false);
			case UnaryExpr::NOT: return g_builder->CreateNot(m_expr->generate());
		default:
			break;
		}
	} else if (isFloat(btype)) {
		switch (m_op) {
			case UnaryExpr::PLUS: return m_expr->generate();
			case UnaryExpr::MINUS: return g_builder->CreateFNeg(m_expr->generate());
		default:
			break;
		}
	} else if (btype == BasicType::POINTER) {
		switch (m_op) {
			case UnaryExpr::POST_INC: return createIncOrDecrement(btype, true, true, false);
			case UnaryExpr::POST_DEC: return createIncOrDecrement(btype, false, true, false);
			case UnaryExpr::PRE_INC: return createIncOrDecrement(btype, true, false, false);
			case UnaryExpr::PRE_DEC: return createIncOrDecrement(btype, false, false, false);
		default:
			break;
		}
	}

	return nullptr;
}

llvm::Value* UnaryExpr::generateRValue() {
	// Operations for which expression type is not important
	if (m_op == UnaryOp::REF) {
		return m_expr->generateRValue();
	} else if (m_op == UnaryOp::DEREF) {
		llvm::Value* value = m_expr->generate();
		if (m_expr->getType()->basicType == BasicType::OPTIONAL) {
			ASSERT(false, "there must be an error");
		}

		return value;
	} else if (m_op == UnaryOp::REF_CONST) {
		if (m_expr->isRVal()) {
			return m_expr->generateRValue();
		} else {
			llvm::Value* alloc = llvm_utils::createLocalVariable(
				g_function->functionValue, 
				m_expr->getType(), 
				"$ref"
			);

			g_builder->CreateStore(m_expr->generate(), alloc);
			return alloc;
		}
	}

	BasicType btype = m_type->basicType;
	if (isInteger(btype) || btype == BasicType::BOOL) {
		switch (m_op) {
			case UnaryExpr::PRE_INC: return createIncOrDecrement(btype, true, false, true);
			case UnaryExpr::PRE_DEC: return createIncOrDecrement(btype, false, false, true);
		default:
			break;
		}
	} else if (btype == BasicType::POINTER) {
		switch (m_op) {
			case UnaryExpr::PRE_INC: return createIncOrDecrement(btype, true, false, true);
			case UnaryExpr::PRE_DEC: return createIncOrDecrement(btype, false, false, true);
		default:
			break;
		}
	}

	return nullptr;
}

llvm::Value* UnaryExpr::createIncOrDecrement(
	BasicType btype, 
	bool isIncrement,
	bool isPostfix,
	bool isRVal
) {
	llvm::Value* ptr_value = m_expr->generateRValue();
	llvm::Value* value = g_builder->CreateLoad(m_type->to_llvm(), ptr_value);

	u64 offsetSize = 1;
	if (btype == BasicType::POINTER) {
		value = g_builder->CreatePtrToInt(value, llvm::Type::getInt64Ty(g_context));
		offsetSize = ((PointerType*)m_type.get())->elementType->getAlignment();
	}

	llvm::Value* offsetValue = llvm_utils::getConstantInt(
		offsetSize,
		m_type->getBitSize(),
		isSigned(m_type->basicType)
	);

	llvm::Value* inc_value;
	if (isIncrement) {
		inc_value = g_builder->CreateAdd(value, offsetValue);
	} else {
		inc_value = g_builder->CreateSub(value, offsetValue);
	}

	if (btype == BasicType::POINTER) {
		inc_value = g_builder->CreateIntToPtr(inc_value, m_type->to_llvm());
	}

	g_builder->CreateStore(inc_value, ptr_value);
	if (isPostfix) {
		if (btype == BasicType::POINTER) {
			return g_builder->CreateIntToPtr(value, m_type->to_llvm());
		} else {
			return value;
		}
	} else {
		if (isRVal) {
			return ptr_value;
		} else {
			return inc_value;
		}
	}
}
