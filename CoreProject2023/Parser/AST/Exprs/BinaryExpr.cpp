#include "BinaryExpr.h"
#include <Parser/Visitor/Visitor.h>
#include <Utils/ErrorManager.h>
#include <Module/LLVMUtils.h>

BinaryExpr::BinaryExpr(std::unique_ptr<Expression> right, std::unique_ptr<Expression> left, TokenType op)
	: m_right(std::move(right)), m_left(std::move(left)), m_op(op) {
	auto& rightType = m_right->getType();
	auto& leftType = m_left->getType();
	switch (op) {
		case TokenType::PLUS:
		case TokenType::MINUS:
		case TokenType::STAR:
		case TokenType::PERCENT:
		case TokenType::POWER:
		case TokenType::AND:
		case TokenType::OR:
		case TokenType::XOR:
		case TokenType::DSLASH:
			m_type = findCommonType(rightType, leftType, m_right->isCompileTime(), m_left->isCompileTime());
			break;
		case TokenType::SLASH: {
			m_type = findCommonType(rightType, leftType, m_right->isCompileTime(), m_left->isCompileTime());
			if (isInteger(m_type->basicType)) {
				m_type = std::make_unique<Type>(BasicType::F64);
			}
			}; break;
		case TokenType::LSHIFT:
		case TokenType::RSHIFT:
			m_type = rightType->copy();
			break;
		case TokenType::ANDAND:
		case TokenType::OROR:
			m_type = std::make_unique<Type>(BasicType::BOOL);
			break;
	default:
		ASSERT(false, "unknown operator");
		break;
	}

	if (!m_type) {
		ErrorManager::typeError(ErrorID::E3103_CANNOT_CONVERT_TO_ONE, m_errLine,
			rightType->toString() + " and " + leftType->toString());
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
			case TokenType::STAR: return g_builder->CreateMul(right, left);
			case TokenType::AND:
			case TokenType::ANDAND: return g_builder->CreateAnd(right, left);
			case TokenType::OR:
			case TokenType::OROR: return g_builder->CreateOr(right, left);
		default:
			ASSERT(false, "wrong operator");
			break;
		}
	}

	// Type conversion
	if (m_op < TokenType::EXCLEQ) {
		if (m_type->basicType == BasicType::POINTER) {
			std::unique_ptr<Type> uint64T = std::make_unique<Type>(BasicType::U64);
			right = llvm_utils::convertValueTo(uint64T, m_right->getType(), right);
			left = llvm_utils::convertValueTo(uint64T, m_left->getType(), left);
		} else {
			right = llvm_utils::convertValueTo(m_type, m_right->getType(), right);
			left = llvm_utils::convertValueTo(m_type, m_left->getType(), left);
		}
	} else {
		std::unique_ptr<Type> commonType = findCommonType(m_right->getType(), m_left->getType(), m_right->isCompileTime(), m_left->isCompileTime());
		right = llvm_utils::convertValueTo(commonType, m_right->getType(), right);
		left = llvm_utils::convertValueTo(commonType, m_left->getType(), left);
	}

	if (isInteger(m_type->basicType)) {
		switch (m_op) {
			case TokenType::PLUS: return g_builder->CreateAdd(right, left);
			case TokenType::MINUS: return g_builder->CreateSub(right, left);
			case TokenType::STAR: return g_builder->CreateMul(right, left);
			case TokenType::PERCENT: return isSigned(m_type->basicType) ?
												g_builder->CreateSRem(right, left)
												: g_builder->CreateURem(right, left);
			case TokenType::POWER: // TODO: implement
			case TokenType::AND: return g_builder->CreateAnd(right, left);
			case TokenType::OR: return g_builder->CreateOr(right, left);
			case TokenType::XOR: return g_builder->CreateXor(right, left);
			case TokenType::DSLASH: return isSigned(m_type->basicType) ?
												g_builder->CreateSDiv(right, left)
												: g_builder->CreateUDiv(right, left);
			case TokenType::LSHIFT: return g_builder->CreateShl(right, left);
			case TokenType::RSHIFT: return g_builder->CreateLShr(right, left);
		default:
			ASSERT(false, "unknown operator");
			break;
		}
	} else if (isFloat(m_type->basicType)) {
		switch (m_op) {
			case TokenType::PLUS: return g_builder->CreateFAdd(right, left);
			case TokenType::MINUS: return g_builder->CreateFSub(right, left);
			case TokenType::STAR: return g_builder->CreateFMul(right, left);
			case TokenType::PERCENT: // TODO: implement
			case TokenType::POWER: // TODO: implement
			case TokenType::DSLASH:
			case TokenType::SLASH: return g_builder->CreateFDiv(right, left);
		default:
			ASSERT(false, "unknown operator");
			break;
		}
	} else if (isString(m_type->basicType)) {
		// TODO: implement
	} else if (m_type->basicType == BasicType::POINTER) {
		switch (m_op) {
			case TokenType::PLUS: right = g_builder->CreateAdd(right, left); break;
			case TokenType::MINUS: right = g_builder->CreateSub(right, left); break;
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
	ErrorManager::parserError(ErrorID::E2103_NOT_A_REFERENCE, m_errLine, "binary operator cannot return a reference");
	return nullptr;
}
