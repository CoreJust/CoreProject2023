#include "AssignmentExpr.h"
#include <Parser/Visitor/Visitor.h>
#include <Module/Module.h>
#include <Module/LLVMUtils.h>
#include <Module/LLVMGlobals.h>
#include <Utils/ErrorManager.h>
#include "BinaryExpr.h"
#include "FunctionCallExpr.h"

std::string AssignmentExpr::assignmentOpToString(AssignmentOp op) {
	switch (op)	{
		case AssignmentExpr::EQUATE: return "=";
		case AssignmentExpr::PLUS_EQ: return "+=";
		case AssignmentExpr::MINUS_EQ: return "-=";
		case AssignmentExpr::MULT_EQ: return "*=";
		case AssignmentExpr::POWER_EQ: return "**=";
		case AssignmentExpr::DIV_EQ: return "/=";
		case AssignmentExpr::IDIV_EQ: return "//=";
		case AssignmentExpr::MOD_EQ: return "%=";
		case AssignmentExpr::LSHIFT_EQ: return "<<=";
		case AssignmentExpr::RSHIFT_EQ: return ">>=";
		case AssignmentExpr::AND_EQ: return "&=";
		case AssignmentExpr::OR_EQ: return "|=";
		case AssignmentExpr::XOR_EQ: return "^=";
	default: return "";
	}
}

AssignmentExpr::AssignmentExpr(
	std::unique_ptr<Expression> lval,
	std::unique_ptr<Expression> expr,
	AssignmentOp op
) : m_lval(std::move(lval)), m_expr(std::move(expr)), m_op(op) {
	if (m_lval->getType()->isConst) {
		ErrorManager::typeError(ErrorID::E3057_IS_A_CONSTANT, m_errLine, "tried to assign a value to a constant");
	} else if (!isTrueReference(m_lval->getType()->basicType)) {
		ErrorManager::typeError(
			ErrorID::E3056_MUST_BE_A_REFERENCE,
			m_errLine,
			"cannot assign value to a non-reference value"
		);
	}
	
	// Looking for operator-function if there is such
	if (Type::dereference(m_lval->getType())->basicType >= BasicType::STR8) {
		std::vector<std::shared_ptr<Type>> argTypes = { m_lval->getType(), m_expr->getType() };
		if (Function* operFunc = g_module->chooseOperator(
			assignmentOpToString(m_op),
			argTypes,
			{ m_lval->isCompileTime(), m_expr->isCompileTime() }
		)) {
			m_operatorFunc = operFunc;
			m_type = m_operatorFunc->prototype.getReturnType();

			return;
		}
	}
		
	m_type = m_lval->getType();
}

void AssignmentExpr::accept(Visitor* visitor, std::unique_ptr<Expression>& node) {
	visitor->visit(this, node);
}

llvm::Value* AssignmentExpr::generate() {
	if (m_operatorFunc != nullptr) {
		std::vector<std::unique_ptr<Expression>> args;
		args.push_back(std::move(m_lval));
		args.push_back(std::move(m_expr));
		return FunctionCallExpr::makeFunctionCall(
			m_operatorFunc->getValue(),
			m_operatorFunc->prototype.genType().get(),
			args,
			m_errLine
		);
	}

	llvm::Value* lval = m_lval->generate();
	llvm::Value* value;
	if (m_op != AssignmentOp::EQUATE) {
		value = BinaryExpr::generateBinaryOperation(m_lval, m_expr, Type::dereference(m_type), BinaryExpr::BinaryOp(m_op - 1), true);
	} else {
		value = m_expr->generate();

		value = llvm_utils::tryImplicitlyConvertTo(
			Type::dereference(m_lval->getType()),
			m_expr->getType(),
			value,
			m_errLine,
			m_expr->isCompileTime()
		);
	}

	g_builder->CreateStore(value, lval);
	return lval;
}
