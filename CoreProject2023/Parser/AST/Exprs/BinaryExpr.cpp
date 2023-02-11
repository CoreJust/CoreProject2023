#include "BinaryExpr.h"
#include <Parser/Visitor/Visitor.h>
#include <Utils/ErrorManager.h>
#include <Module/LLVMUtils.h>
#include <Module/LLVMGlobals.h>

BinaryExpr::BinaryExpr(
	std::unique_ptr<Expression> right, 
	std::unique_ptr<Expression> left, 
	BinaryOp op
) : 
	m_right(std::move(right)), 
	m_left(std::move(left)), 
	m_op(op) 
{
	auto& rightType = m_right->getType();
	auto& leftType = m_left->getType();
	switch (op) {
		case BinaryOp::PLUS:
		case BinaryOp::MINUS:
		case BinaryOp::MULT:
		case BinaryOp::MOD:
		case BinaryOp::POWER:
		case BinaryOp::AND:
		case BinaryOp::OR:
		case BinaryOp::XOR:
		case BinaryOp::IDIV:
			m_type = findCommonType(rightType, leftType, m_right->isCompileTime(), m_left->isCompileTime());
			break;
		case BinaryOp::DIV: {
			m_type = findCommonType(rightType, leftType, m_right->isCompileTime(), m_left->isCompileTime());
			if (isInteger(m_type->basicType)) {
				m_type = std::make_unique<Type>(BasicType::F64);
			}
			}; break;
		case BinaryOp::LSHIFT:
		case BinaryOp::RSHIFT:
			m_type = rightType->copy();
			break;
		case BinaryOp::LOGICAL_AND:
		case BinaryOp::LOGICAL_OR:
			m_type = std::make_unique<Type>(BasicType::BOOL);
			break;
	default:
		ASSERT(false, "unknown operator");
		break;
	}

	if (!m_type) {
		ErrorManager::typeError(
			ErrorID::E3103_CANNOT_CONVERT_TO_ONE, 
			m_errLine,
			rightType->toString() + " and " + leftType->toString()
		);
	} else if (!m_isRVal && isReference(m_type->basicType)) {
		PointerType* ptrType = (PointerType*)m_type.get();
		std::unique_ptr<Type> tmp = ptrType->elementType->copy();
		m_type = std::move(tmp);
	}
}

void BinaryExpr::accept(Visitor* visitor, std::unique_ptr<Expression>& node) {
	visitor->visit(this, node);
}

llvm::Value* BinaryExpr::generate() {
	llvm::Value* right = m_right->generate();
	llvm::Value* left = m_left->generate();

	if (m_type->basicType == BasicType::BOOL) {
		right = llvm_utils::convertToBool(m_right->getType(), right);
		left = llvm_utils::convertToBool(m_left->getType(), left);

		switch(m_op) {
			case BinaryOp::MULT: return g_builder->CreateMul(right, left);
			case BinaryOp::AND:
			case BinaryOp::LOGICAL_AND: return g_builder->CreateAnd(right, left);
			case BinaryOp::OR:
			case BinaryOp::LOGICAL_OR: return g_builder->CreateOr(right, left);
		default:
			ASSERT(false, "wrong operator");
			break;
		}
	}

	// Type conversion
	if (m_op < BinaryOp::LOGICAL_AND) {
		if (m_type->basicType == BasicType::POINTER) {
			std::unique_ptr<Type> uint64T = std::make_unique<Type>(BasicType::U64);
			right = llvm_utils::convertValueTo(uint64T, m_right->getType(), right);
			left = llvm_utils::convertValueTo(uint64T, m_left->getType(), left);
		} else {
			right = llvm_utils::convertValueTo(m_type, m_right->getType(), right);
			left = llvm_utils::convertValueTo(m_type, m_left->getType(), left);
		}
	} else {
		std::unique_ptr<Type> commonType = findCommonType(
			m_right->getType(), 
			m_left->getType(), 
			m_right->isCompileTime(),
			m_left->isCompileTime()
		);

		right = llvm_utils::convertValueTo(commonType, m_right->getType(), right);
		left = llvm_utils::convertValueTo(commonType, m_left->getType(), left);
	}

	if (isInteger(m_type->basicType)) {
		switch (m_op) {
			case BinaryOp::PLUS: return g_builder->CreateAdd(right, left);
			case BinaryOp::MINUS: return g_builder->CreateSub(right, left);
			case BinaryOp::MULT: return g_builder->CreateMul(right, left);
			case BinaryOp::MOD: return isSigned(m_type->basicType) ?
												g_builder->CreateSRem(right, left)
												: g_builder->CreateURem(right, left);
			case BinaryOp::POWER: // TODO: implement
			case BinaryOp::AND: return g_builder->CreateAnd(right, left);
			case BinaryOp::OR: return g_builder->CreateOr(right, left);
			case BinaryOp::XOR: return g_builder->CreateXor(right, left);
			case BinaryOp::IDIV: return isSigned(m_type->basicType) ?
												g_builder->CreateSDiv(right, left)
												: g_builder->CreateUDiv(right, left);
			case BinaryOp::LSHIFT: return g_builder->CreateShl(right, left);
			case BinaryOp::RSHIFT: return g_builder->CreateLShr(right, left);
		default:
			ASSERT(false, "unknown operator");
			break;
		}
	} else if (isFloat(m_type->basicType)) {
		switch (m_op) {
			case BinaryOp::PLUS: return g_builder->CreateFAdd(right, left);
			case BinaryOp::MINUS: return g_builder->CreateFSub(right, left);
			case BinaryOp::MULT: return g_builder->CreateFMul(right, left);
			case BinaryOp::MOD: // TODO: implement
			case BinaryOp::POWER: // TODO: implement
			case BinaryOp::IDIV:
			case BinaryOp::DIV: return g_builder->CreateFDiv(right, left);
		default:
			ASSERT(false, "unknown operator");
			break;
		}
	} else if (isString(m_type->basicType)) {
		// TODO: implement
	} else if (m_type->basicType == BasicType::POINTER) {
		u64 typeSize = ((PointerType*)m_type.get())->elementType->getAlignment();
		if (isInteger(m_right->getType()->basicType)) {
			right = g_builder->CreateMul(
				right, 
				llvm_utils::getConstantInt(typeSize, 64)
			);
		} else if (isInteger(m_left->getType()->basicType)) {
			left = g_builder->CreateMul(
				left, 
				llvm_utils::getConstantInt(typeSize, 64)
			);
		}

		switch (m_op) {
			case BinaryOp::PLUS: right = g_builder->CreateAdd(right, left); break;
			case BinaryOp::MINUS: right = g_builder->CreateSub(right, left); break;
		default:
			ASSERT(false, "unknown operator");
			break;
		}

		std::unique_ptr<Type> uint64T = std::make_unique<Type>(BasicType::U64);
		return llvm_utils::convertValueTo(m_type, uint64T, right);
	}

	return nullptr;
}

llvm::Value* BinaryExpr::generateRValue() {
	ErrorManager::parserError(
		ErrorID::E2103_NOT_A_REFERENCE, 
		m_errLine, 
		"binary operator cannot return a reference"
	);

	return nullptr;
}
