#include "TernaryExpr.h"
#include <Parser/Visitor/Visitor.h>
#include <Utils/ErrorManager.h>
#include <Module/LLVMUtils.h>
#include <Module/LLVMGlobals.h>

TernaryExpr::TernaryExpr(std::unique_ptr<Expression> condition, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right)
	: m_condition(std::move(condition)), m_left(std::move(left)), m_right(std::move(right)) {
	m_type = findCommonType(m_left->getType(), m_right->getType());

	if (!m_type) {
		ErrorManager::typeError(
			ErrorID::E3103_CANNOT_CONVERT_TO_ONE, 
			m_errLine, 
			"ternary expression's options must be of the same type: " + toString()
		);
	}
}

void TernaryExpr::accept(Visitor* visitor, std::unique_ptr<Expression>& node) {
	visitor->visit(this, node);
}

llvm::Value* TernaryExpr::generate() {
	llvm::Value* condVal = m_condition->generate();
	condVal = llvm_utils::convertToBool(m_condition->getType(), condVal);
	ASSERT(condVal, "cannot be null");

	llvm::Function* fun = g_builder->GetInsertBlock()->getParent();
	llvm::BasicBlock* ifBlock = llvm::BasicBlock::Create(g_context, "ternary_if", fun);
	llvm::BasicBlock* elseBlock = llvm::BasicBlock::Create(g_context, "ternary_else");
	llvm::BasicBlock* mergeBlock = llvm::BasicBlock::Create(g_context, "ternary_merge");

	llvm::Value* leftVal = nullptr, *rightVal = nullptr;

	g_builder->CreateCondBr(condVal, ifBlock, elseBlock);

	// left option
	g_builder->SetInsertPoint(ifBlock);

	leftVal = m_left->generate();
	leftVal = llvm_utils::tryImplicitlyConvertTo(
		m_type,
		m_left->getType(),
		leftVal,
		m_errLine,
		m_left->isCompileTime()
	);

	g_builder->CreateBr(mergeBlock);

	// right option
	ifBlock = g_builder->GetInsertBlock();
	fun->getBasicBlockList().push_back(elseBlock);
	g_builder->SetInsertPoint(elseBlock);

	rightVal = m_right->generate();
	rightVal = llvm_utils::tryImplicitlyConvertTo(
		m_type,
		m_right->getType(),
		rightVal,
		m_errLine,
		m_right->isCompileTime()
	);

	g_builder->CreateBr(mergeBlock);

	// merge and PHI
	elseBlock = g_builder->GetInsertBlock();
	fun->getBasicBlockList().push_back(mergeBlock);
	g_builder->SetInsertPoint(mergeBlock);

	llvm::PHINode* phi = g_builder->CreatePHI(m_type->to_llvm(), 2);
	phi->addIncoming(leftVal, ifBlock);
	phi->addIncoming(rightVal, elseBlock);

	return phi;
}

std::string TernaryExpr::toString() const {
	return "(" + m_condition->toString() + " ? " + m_left->toString() + " : " + m_right->toString() + ")";
}
