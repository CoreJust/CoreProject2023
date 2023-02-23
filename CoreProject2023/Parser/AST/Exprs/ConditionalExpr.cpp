#include "ConditionalExpr.h"
#include <Parser/Visitor/Visitor.h>
#include <Utils/ErrorManager.h>
#include <Module/Module.h>
#include <Module/LLVMUtils.h>
#include <Module/LLVMGlobals.h>
#include "FunctionCallExpr.h"

std::string ConditionalExpr::conditionOpToString(ConditionOp op) {
	switch (op) {
		case ConditionalExpr::EQUALS: return "==";
		case ConditionalExpr::NOT_EQUALS: return "!=";
		case ConditionalExpr::LESS: return "<";
		case ConditionalExpr::GREATER: return ">";
		case ConditionalExpr::LESS_OR_EQUAL: return "<=";
		case ConditionalExpr::GREATER_OR_EQUAL: return ">=";
	default: return "";
	}
}

ConditionalExpr::ConditionalExpr(std::vector<std::unique_ptr<Expression>> exprs, std::vector<ConditionOp> ops)
	: m_exprs(std::move(exprs)), m_ops(std::move(ops)) {
	ASSERT(m_exprs.size() == m_ops.size() + 1, "incorrect conditional expr");
	m_type = std::make_unique<Type>(BasicType::BOOL);

	m_operatorFuncs.resize(m_ops.size(), nullptr);
	for (size_t i = 0; i < m_ops.size(); i++) {
		std::unique_ptr<Expression>& left = m_exprs[i];
		std::unique_ptr<Expression>& right = m_exprs[i + 1];

		if (Type::getTheVeryType(left->getType())->basicType >= BasicType::STR8
			|| Type::getTheVeryType(right->getType())->basicType >= BasicType::STR8) {
			std::vector<std::unique_ptr<Type>> argTypes;
			argTypes.push_back(left->getType()->copy());
			argTypes.push_back(right->getType()->copy());
			if (Function* operFunc = g_module->chooseOperator(
				conditionOpToString(m_ops[i]),
				argTypes,
				{ left->isCompileTime(), right->isCompileTime() }
			)) {
				m_operatorFuncs[i] = operFunc;
				if (operFunc->prototype.getReturnType()->basicType != BasicType::BOOL) {
					ErrorManager::parserError(
						ErrorID::E2109_CONDITIONAL_OPERATOR_MUST_RETURN_BOOL,
						m_errLine,
						"operator: " + operFunc->prototype.toString()
					);
				}
			}
		}
	}
}

void ConditionalExpr::accept(Visitor* visitor, std::unique_ptr<Expression>& node) {
	visitor->visit(this, node);
}

llvm::Value* ConditionalExpr::generate() {
	std::vector<llvm::Value*> values;
	values.resize(m_ops.size(), nullptr);

	// Comparing the expressions
	llvm::Value* orig_left = m_exprs[0]->generate();
	for (size_t i = 0; i < m_exprs.size() - 1; i++) {
		llvm::Value* orig_right = m_exprs[i + 1]->generate();

		if (m_operatorFuncs[i] != nullptr) {
			std::vector<llvm::Value*> args;
			std::vector<std::unique_ptr<Type>> argTypes;
			std::vector<bool> isCompileTime;

			args.push_back(orig_left);
			argTypes.push_back(m_exprs[i]->getType()->copy());
			isCompileTime.push_back(m_exprs[i]->isCompileTime());

			args.push_back(orig_right);
			argTypes.push_back(m_exprs[i + 1]->getType()->copy());
			isCompileTime.push_back(m_exprs[i + 1]->isCompileTime());
			values[i] = FunctionCallExpr::makeFunctionCall(
				m_operatorFuncs[i]->getValue(),
				m_operatorFuncs[i]->prototype.genType().get(),
				args,
				argTypes,
				isCompileTime,
				m_errLine
			);

			continue;
		}

		// Default operators
		std::unique_ptr<Type> commonType = findCommonType(
			m_exprs[i]->getType(),
			m_exprs[i + 1]->getType(),
			m_exprs[i]->isCompileTime(),
			m_exprs[i + 1]->isCompileTime()
		);

		if (isTruePointer(commonType->basicType)) {
			commonType = std::make_unique<Type>(BasicType::U64);
		}

		llvm::Value* left = llvm_utils::convertValueTo(commonType, m_exprs[i]->getType(), orig_left);
		llvm::Value* right = llvm_utils::convertValueTo(commonType, m_exprs[i + 1]->getType(), orig_right);

		if (isInteger(commonType->basicType) 
			|| isChar(commonType->basicType)
			|| commonType->basicType == BasicType::BOOL) {
			bool isUnsigned = ::isUnsigned(commonType->basicType);
			switch (m_ops[i]) {
				case ConditionOp::EQUALS: values[i] = g_builder->CreateICmpEQ(left, right); break;
				case ConditionOp::NOT_EQUALS: values[i] = g_builder->CreateICmpNE(left, right); break;
				case ConditionOp::LESS:
					values[i] = isUnsigned ?
						g_builder->CreateICmpULT(left, right)
						: g_builder->CreateICmpSLT(left, right);
					break;
				case ConditionOp::GREATER:
					values[i] = isUnsigned ?
						g_builder->CreateICmpUGT(left, right)
						: g_builder->CreateICmpSGT(left, right);
					break;
				case ConditionOp::LESS_OR_EQUAL:
					values[i] = isUnsigned ?
						g_builder->CreateICmpULE(left, right)
						: g_builder->CreateICmpSLE(left, right);
					break;
				case ConditionOp::GREATER_OR_EQUAL:
					values[i] = isUnsigned ?
						g_builder->CreateICmpUGE(left, right)
						: g_builder->CreateICmpSGE(left, right);
					break;
			default:
				ASSERT(false, "unknown conditional operator");
				break;
			}
		} else if (isFloat(commonType->basicType)) {
			switch (m_ops[i]) {
				case ConditionOp::EQUALS: values[i] = g_builder->CreateFCmpUEQ(left, right); break;
				case ConditionOp::NOT_EQUALS: values[i] = g_builder->CreateFCmpUNE(left, right); break;
				case ConditionOp::LESS: values[i] = g_builder->CreateFCmpULT(left, right); break;
				case ConditionOp::GREATER: values[i] = g_builder->CreateFCmpUGT(left, right); break;
				case ConditionOp::LESS_OR_EQUAL: values[i] = g_builder->CreateFCmpULE(left, right); break;
				case ConditionOp::GREATER_OR_EQUAL: values[i] = g_builder->CreateFCmpUGE(left, right); break;
			default:
				ASSERT(false, "unknown conditional operator");
				break;
			}
		} else {
			// TODO: implement
		}
	}

	// Generating the overall result
	llvm::Value* result = values[0];
	for (size_t i = 1; i < values.size() - 1; i++) {
		result = g_builder->CreateAnd(result, values[i]);
	}

	return result;
}
