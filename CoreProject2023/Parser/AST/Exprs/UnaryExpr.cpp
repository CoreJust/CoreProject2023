#include "UnaryExpr.h"
#include <Parser/Visitor/Visitor.h>
#include <Utils/ErrorManager.h>
#include <Module/Module.h>
#include <Module/LLVMUtils.h>
#include <Module/LLVMGlobals.h>
#include "FunctionCallExpr.h"

std::string UnaryExpr::unaryOpToString(UnaryOp op) {
	switch (op) {
		case UnaryExpr::PLUS: return "+";
		case UnaryExpr::MINUS: return "-";
		case UnaryExpr::POST_INC: return "++";
		case UnaryExpr::POST_DEC: return "--";
		case UnaryExpr::PRE_INC: return "++";
		case UnaryExpr::PRE_DEC: return "--";
		case UnaryExpr::NOT: return "~";
		case UnaryExpr::LOGICAL_NOT: return "!";
		case UnaryExpr::ADRESS: return "&";
		case UnaryExpr::DEREF: return "*";
		case UnaryExpr::MOVE: return "move";
	default: return "";
	}
}

bool UnaryExpr::isUnaryOpDefinable(UnaryOp op) {
	switch (op) {
		case UnaryExpr::PLUS: return true;
		case UnaryExpr::MINUS: return true;
		case UnaryExpr::POST_INC: return true;
		case UnaryExpr::POST_DEC: return true;
		case UnaryExpr::PRE_INC: return true;
		case UnaryExpr::PRE_DEC: return true;
		case UnaryExpr::NOT: return true;
		case UnaryExpr::LOGICAL_NOT: return true;
		case UnaryExpr::ADRESS: return true;
		case UnaryExpr::DEREF: return true;
		case UnaryExpr::MOVE: return false;
	default: return false;
	}
}

UnaryExpr::UnaryExpr(std::unique_ptr<Expression> expr, UnaryOp op)
	: m_expr(std::move(expr)), m_op(op) {
	if (isUnaryOpDefinable(m_op) && Type::dereference(m_expr->getType())->basicType >= BasicType::STR8) {
		std::vector<std::shared_ptr<Type>> argTypes;
		argTypes.push_back(m_expr->getType());
		if (Function* operFunc = g_module->chooseOperator(
			unaryOpToString(m_op),
			argTypes,
			{ m_expr->isCompileTime() },
			m_op == UnaryOp::PRE_INC || m_op == UnaryOp::PRE_DEC // whether the function must return a reference type
		)) {
			m_operatorFunc = operFunc;
			m_type = m_operatorFunc->prototype.getReturnType();
			m_safety = operFunc->prototype.getQualities().getSafety();
			g_safety.tryUse(m_safety, m_errLine);

			return;
		}
	}

	switch (m_op) {
		case UnaryExpr::PLUS:
		case UnaryExpr::NOT:
			m_type = Type::dereference(m_expr->getType());
			break;
		case UnaryExpr::MINUS:
			m_type = Type::dereference(m_expr->getType());
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
		case UnaryExpr::POST_INC:
		case UnaryExpr::POST_DEC:
			if (!m_expr->isLVal()) {
				ErrorManager::typeError(
					ErrorID::E3056_MUST_BE_A_REFERENCE,
					m_errLine,
					"increment/decrement of " + m_type->toString()
				);
			}

			if (m_op == UnaryOp::PRE_DEC || m_op == UnaryOp::PRE_INC) {
				m_type = m_expr->getType();
			} else {
				m_type = m_expr->getType()->asPointerType()->elementType;
			}

			break;
		case UnaryExpr::LOGICAL_NOT:
			m_type = Type::createType(BasicType::BOOL);
			break;
		case UnaryExpr::ADRESS:
			m_type = PointerType::createType(BasicType::POINTER, Type::dereference(m_expr->getType()));
			break;
		case UnaryExpr::DEREF:
			if (auto btype = Type::dereference(m_expr->getType())->basicType;
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
				if (Type::dereference(m_expr->getType())->basicType == BasicType::ARRAY) {
					m_type = PointerType::createType(
						BasicType::LVAL_REFERENCE,
						Type::dereference(m_expr->getType())->asArrayType()->elementType,
						Type::dereference(m_expr->getType())->isConst
					);
				} else {
					m_type = PointerType::createType(
						BasicType::LVAL_REFERENCE,
						Type::dereference(m_expr->getType())->asPointerType()->elementType,
						Type::dereference(m_expr->getType())->isConst
					);
				}
			}

			m_safety = Safety::UNSAFE;
			g_safety.tryUse(m_safety, m_errLine);

			break;
		case UnaryExpr::MOVE:
			m_type = PointerType::createType(BasicType::RVAL_REFERENCE, Type::dereference(m_expr->getType()));
			break;
	default:
		ASSERT(false, "wrong operator");
		break;
	}
}

void UnaryExpr::accept(Visitor* visitor, std::unique_ptr<Expression>& node) {
	visitor->visit(this, node);
}

llvm::Value* UnaryExpr::generate() {
	if (m_operatorFunc) {
		std::vector<std::unique_ptr<Expression>> args;
		args.push_back(std::move(m_expr));
		return FunctionCallExpr::makeFunctionCall(
			m_operatorFunc->getValue(),
			m_operatorFunc->prototype.genType().get(),
			args,
			m_errLine
		);
	}

	// Operations for which expression type is not important
	if (m_op == UnaryOp::ADRESS) {
		return m_expr->generate();
	} else if (m_op == UnaryOp::DEREF) {
		if (m_expr->getType()->basicType == BasicType::OPTIONAL) {

		}

		return m_expr->generate();
	} else if (m_op == UnaryOp::MOVE) {
		llvm::Value* value = m_expr->generate();
		const std::shared_ptr<Type>& type = isPointer(m_expr->getType()->basicType) ?
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

	auto genExpr = [&]() -> llvm::Value* {
		llvm::Value* result = m_expr->generate();
		return llvm_utils::convertValueTo(
			Type::dereference(m_expr->getType()),
			m_expr->getType(),
			result
		);
	};
	
	BasicType btype = Type::dereference(m_type)->basicType;
	if (isInteger(btype) || btype == BasicType::BOOL) {
		switch (m_op) {
			case UnaryExpr::PLUS: return genExpr();
			case UnaryExpr::MINUS: return g_builder->CreateNeg(genExpr());
			case UnaryExpr::POST_INC: return createIncOrDecrement(btype, true, true, true);
			case UnaryExpr::POST_DEC: return createIncOrDecrement(btype, false, true, true);
			case UnaryExpr::PRE_INC: return createIncOrDecrement(btype, true, false, true);
			case UnaryExpr::PRE_DEC: return createIncOrDecrement(btype, false, false, true);
			case UnaryExpr::NOT: return g_builder->CreateNot(genExpr());
		default:
			break;
		}
	} else if (isFloat(btype)) {
		switch (m_op) {
			case UnaryExpr::PLUS: return genExpr();
			case UnaryExpr::MINUS: return g_builder->CreateFNeg(genExpr());
		default:
			break;
		}
	} else if (btype == BasicType::POINTER) {
		switch (m_op) {
			case UnaryExpr::POST_INC: return createIncOrDecrement(btype, true, true, true);
			case UnaryExpr::POST_DEC: return createIncOrDecrement(btype, false, true, true);
			case UnaryExpr::PRE_INC: return createIncOrDecrement(btype, true, false, true);
			case UnaryExpr::PRE_DEC: return createIncOrDecrement(btype, false, false, true);
		default:
			break;
		}
	}

	return nullptr;
}

std::string UnaryExpr::toString() const {
	std::string result = "";
	if (m_op != UnaryOp::POST_DEC && m_op != UnaryOp::POST_INC) {
		result += unaryOpToString(m_op);
	}

	if (m_op == UnaryOp::MOVE) {
		result += ' ';
	}

	result += m_expr->toString();

	if (m_op == UnaryOp::POST_DEC || m_op == UnaryOp::POST_INC) {
		result += unaryOpToString(m_op);
	}

	return result;
}

llvm::Value* UnaryExpr::createIncOrDecrement(
	BasicType btype, 
	bool isIncrement,
	bool isPostfix,
	bool isRVal
) {
	const std::shared_ptr<Type>& type = isPostfix ? m_type : m_type->asPointerType()->elementType;

	llvm::Value* ptr_value = m_expr->generate();
	llvm::Value* value = g_builder->CreateLoad(type->to_llvm(), ptr_value);

	u64 offsetSize = 1;
	if (btype == BasicType::POINTER) {
		value = g_builder->CreatePtrToInt(value, llvm::Type::getInt64Ty(g_context));
		offsetSize = m_type->asPointerType()->elementType->getAlignment();
	}

	llvm::Value* offsetValue = llvm_utils::getConstantInt(
		offsetSize,
		type->getBitSize(),
		isSigned(type->basicType)
	);

	llvm::Value* inc_value;
	if (isIncrement) {
		inc_value = g_builder->CreateAdd(value, offsetValue);
	} else {
		inc_value = g_builder->CreateSub(value, offsetValue);
	}

	if (btype == BasicType::POINTER) {
		inc_value = g_builder->CreateIntToPtr(inc_value, type->to_llvm());
	}

	g_builder->CreateStore(inc_value, ptr_value);
	if (isPostfix) {
		if (btype == BasicType::POINTER) {
			return g_builder->CreateIntToPtr(value, type->to_llvm());
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
