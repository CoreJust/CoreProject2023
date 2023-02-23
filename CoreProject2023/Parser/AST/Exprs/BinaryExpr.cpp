#include "BinaryExpr.h"
#include <Parser/Visitor/Visitor.h>
#include <Utils/ErrorManager.h>
#include <Module/Module.h>
#include <Module/LLVMUtils.h>
#include <Module/LLVMGlobals.h>
#include "FunctionCallExpr.h"

std::string BinaryExpr::binaryOpToString(BinaryOp op) {
	switch (op) {
		case BinaryExpr::PLUS: return "+";
		case BinaryExpr::MINUS: return "-";
		case BinaryExpr::MULT: return "*";
		case BinaryExpr::POWER: return "**";
		case BinaryExpr::DIV: return "/";
		case BinaryExpr::IDIV: return "//";
		case BinaryExpr::MOD: return "%";
		case BinaryExpr::LSHIFT: return "<<";
		case BinaryExpr::RSHIFT: return ">>";
		case BinaryExpr::AND: return "&";
		case BinaryExpr::OR: return "|";
		case BinaryExpr::XOR: return "^";
	default: return "";
	}
}

bool BinaryExpr::isBinaryOpDefinable(BinaryOp op) {
	switch (op) {
		case BinaryExpr::PLUS:
		case BinaryExpr::MINUS:
		case BinaryExpr::MULT:
		case BinaryExpr::POWER:
		case BinaryExpr::DIV:
		case BinaryExpr::IDIV:
		case BinaryExpr::MOD:
		case BinaryExpr::LSHIFT:
		case BinaryExpr::RSHIFT:
		case BinaryExpr::AND:
		case BinaryExpr::OR:
		case BinaryExpr::XOR: return true;
	default: return false;
	}
}

BinaryExpr::BinaryExpr(
	std::unique_ptr<Expression> left,
	std::unique_ptr<Expression> right,
	BinaryOp op
) :
	m_left(std::move(left)),
	m_right(std::move(right)), 
	m_op(op) 
{
	auto& rightType = m_right->getType();
	auto& leftType = m_left->getType();
	if (isBinaryOpDefinable(m_op)
		&& (Type::getTheVeryType(rightType)->basicType >= BasicType::STR8
			|| Type::getTheVeryType(leftType)->basicType >= BasicType::STR8)) {
		std::vector<std::unique_ptr<Type>> argTypes;
		argTypes.push_back(leftType->copy());
		argTypes.push_back(rightType->copy());
		if (Function* operFunc = g_module->chooseOperator(
			binaryOpToString(m_op),
			argTypes,
			{ m_left->isCompileTime(), m_right->isCompileTime() }
		)) {
			m_operatorFunc = operFunc;
			m_type = m_operatorFunc->prototype.getReturnType()->copy();

			return;
		}
	}

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
			m_type = leftType->copy();
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
			leftType->toString() + " and " + rightType->toString()
		);
	} else if (!isLVal() && isReference(m_type->basicType)) {
		std::unique_ptr<Type> tmp = m_type->asPointerType()->elementType->copy();
		m_type = std::move(tmp);
	}
}

void BinaryExpr::accept(Visitor* visitor, std::unique_ptr<Expression>& node) {
	visitor->visit(this, node);
}

llvm::Value* BinaryExpr::generate() {
	if (m_operatorFunc) {
		std::vector<std::unique_ptr<Expression>> args;
		args.push_back(std::move(m_left));
		args.push_back(std::move(m_right));
		return FunctionCallExpr::makeFunctionCall(
			m_operatorFunc->getValue(),
			m_operatorFunc->prototype.genType().get(),
			args,
			m_errLine
		);
	}

	return generateBinaryOperation(m_left, m_right, m_type, m_op, false);
}

llvm::Value* BinaryExpr::generateBinaryOperation(
	std::unique_ptr<Expression>& left,
	std::unique_ptr<Expression>& right,
	std::unique_ptr<Type>& resultingType,
	BinaryOp op,
	bool convertToResultingType
) {
	llvm::Value* leftVal = left->generate();
	llvm::Value* rightVal = right->generate();

	if (!convertToResultingType) {
		if (resultingType->basicType == BasicType::BOOL) {
			leftVal = llvm_utils::convertToBool(left->getType(), leftVal);
			rightVal = llvm_utils::convertToBool(right->getType(), rightVal);

			switch (op) {
			case BinaryOp::MULT: return g_builder->CreateMul(leftVal, rightVal);
			case BinaryOp::AND:
			case BinaryOp::LOGICAL_AND: return g_builder->CreateAnd(leftVal, rightVal);
			case BinaryOp::OR:
			case BinaryOp::LOGICAL_OR: return g_builder->CreateOr(leftVal, rightVal);
			default:
				ASSERT(false, "wrong operator");
				break;
			}
		}

		// Type conversion
		if (op < BinaryOp::LOGICAL_AND) {
			if (resultingType->basicType != BasicType::POINTER) {
				leftVal = llvm_utils::convertValueTo(resultingType, left->getType(), leftVal);
				rightVal = llvm_utils::convertValueTo(resultingType, right->getType(), rightVal);
			} else {
				std::unique_ptr<Type> uint64T = std::make_unique<Type>(BasicType::U64);
				leftVal = llvm_utils::convertValueTo(uint64T, left->getType(), leftVal);
				rightVal = llvm_utils::convertValueTo(uint64T, right->getType(), rightVal);
			}
		} else {
			std::unique_ptr<Type> commonType = findCommonType(
				right->getType(),
				left->getType(),
				right->isCompileTime(),
				left->isCompileTime()
			);

			rightVal = llvm_utils::convertValueTo(commonType, right->getType(), rightVal);
			leftVal = llvm_utils::convertValueTo(commonType, left->getType(), leftVal);
		}
	} else {
		if (op < BinaryOp::LOGICAL_AND && resultingType->basicType == BasicType::POINTER) { // must be executed anyway
			std::unique_ptr<Type> uint64T = std::make_unique<Type>(BasicType::U64);
			leftVal = llvm_utils::convertValueTo(uint64T, left->getType(), leftVal);
			rightVal = llvm_utils::convertValueTo(uint64T, right->getType(), rightVal);
		} else {
			leftVal = llvm_utils::convertValueTo(resultingType, left->getType(), leftVal);
			rightVal = llvm_utils::convertValueTo(resultingType, right->getType(), rightVal);
		}
	}

	if (isInteger(resultingType->basicType)) {
		switch (op) {
			case BinaryOp::PLUS: return g_builder->CreateAdd(leftVal, rightVal);
			case BinaryOp::MINUS: return g_builder->CreateSub(leftVal, rightVal);
			case BinaryOp::MULT: return g_builder->CreateMul(leftVal, rightVal);
			case BinaryOp::MOD: return isSigned(resultingType->basicType) ?
				g_builder->CreateSRem(leftVal, rightVal)
				: g_builder->CreateURem(leftVal, rightVal);
			case BinaryOp::POWER: // TODO: implement
			case BinaryOp::AND: return g_builder->CreateAnd(leftVal, rightVal);
			case BinaryOp::OR: return g_builder->CreateOr(leftVal, rightVal);
			case BinaryOp::XOR: return g_builder->CreateXor(leftVal, rightVal);
			case BinaryOp::IDIV: return isSigned(resultingType->basicType) ?
				g_builder->CreateSDiv(leftVal, rightVal)
				: g_builder->CreateUDiv(leftVal, rightVal);
			case BinaryOp::LSHIFT: return g_builder->CreateShl(leftVal, rightVal);
			case BinaryOp::RSHIFT: return g_builder->CreateLShr(leftVal, rightVal);
		default:
			ASSERT(false, "unknown operator");
			break;
		}
	} else if (isFloat(resultingType->basicType)) {
		switch (op) {
			case BinaryOp::PLUS: return g_builder->CreateFAdd(leftVal, rightVal);
			case BinaryOp::MINUS: return g_builder->CreateFSub(leftVal, rightVal);
			case BinaryOp::MULT: return g_builder->CreateFMul(leftVal, rightVal);
			case BinaryOp::MOD: // TODO: implement
			case BinaryOp::POWER: // TODO: implement
			case BinaryOp::IDIV:
			case BinaryOp::DIV: return g_builder->CreateFDiv(leftVal, rightVal);
		default:
			ASSERT(false, "unknown operator");
			break;
		}
	} else if (resultingType->basicType == BasicType::POINTER) {
		u64 typeSize = resultingType->asPointerType()->elementType->getAlignment();
		if (isInteger(right->getType()->basicType)) {
			rightVal = g_builder->CreateMul(
				rightVal,
				llvm_utils::getConstantInt(typeSize, 64)
			);
		} else if (isInteger(left->getType()->basicType)) {
			leftVal = g_builder->CreateMul(
				leftVal,
				llvm_utils::getConstantInt(typeSize, 64)
			);
		}

		switch (op) {
			case BinaryOp::PLUS: rightVal = g_builder->CreateAdd(leftVal, rightVal); break;
			case BinaryOp::MINUS: rightVal = g_builder->CreateSub(leftVal, rightVal); break;
		default:
			ASSERT(false, "unknown operator");
			break;
		}

		std::unique_ptr<Type> uint64T = std::make_unique<Type>(BasicType::U64);
		return llvm_utils::convertValueTo(resultingType, uint64T, rightVal);
	}

	ASSERT(false, "something went wrong");
	return nullptr;
}
