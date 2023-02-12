#include "ConditionalExpr.h"
#include <Parser/Visitor/Visitor.h>
#include <Utils/ErrorManager.h>
#include <Module/LLVMUtils.h>
#include <Module/LLVMGlobals.h>

ConditionalExpr::ConditionalExpr(std::vector<std::unique_ptr<Expression>> exprs, std::vector<ConditionOp> ops)
	: m_exprs(std::move(exprs)), m_ops(std::move(ops)) {
	ASSERT(m_exprs.size() == m_ops.size() + 1, "incorrect conditional expr");
	m_type = std::make_unique<Type>(BasicType::BOOL);
}

void ConditionalExpr::accept(Visitor* visitor, std::unique_ptr<Expression>& node) {
	visitor->visit(this, node);
}

llvm::Value* ConditionalExpr::generate() {
	// Looking for type common for all the expressions
	std::unique_ptr<Type> commonType = m_exprs[0]->getType()->copy();
	bool isCompileTime = m_exprs[0]->isCompileTime();
	for (size_t i = 1; i < m_exprs.size(); i++) {
		commonType = findCommonType(commonType, m_exprs[i]->getType(), isCompileTime, m_exprs[i]->isCompileTime());
		isCompileTime = isCompileTime && m_exprs[i]->isCompileTime();
	}

	if (commonType->basicType == BasicType::POINTER
		|| commonType->basicType == BasicType::FUNCTION
		|| commonType->basicType == BasicType::ARRAY) {
		commonType = std::make_unique<Type>(BasicType::U64);
	}

	// Generating the expressions and converting them to common type
	std::vector<llvm::Value*> values;
	for (auto& expr : m_exprs) {
		values.push_back(expr->generate());
		values.back() = llvm_utils::convertValueTo(commonType, expr->getType(), values.back());
	}

	// Comparing the expressions
	for (size_t i = 0; i < values.size() - 1; i++) {
		llvm::Value* left = values[i];
		llvm::Value* right = values[i + 1];

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

llvm::Value* ConditionalExpr::generateRValue() {
	ErrorManager::parserError(
		ErrorID::E2103_NOT_A_REFERENCE,
		m_errLine,
		"conditional operator cannot return a reference"
	);

	return nullptr;
}
