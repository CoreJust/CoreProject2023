#include "IfElseStatement.h"
#include <Parser/Visitor/Visitor.h>
#include <Module/Module.h>
#include <Module/LLVMUtils.h>

IfElseStatement::IfElseStatement(std::vector<std::unique_ptr<Statement>> bodies, std::vector<std::unique_ptr<Expression>> conditions)
	: m_bodies(std::move(bodies)), m_conditions(std::move(conditions)) {
	ASSERT(m_conditions.size() && m_conditions.size() <= m_bodies.size(), "Wrong if statement");
}

void IfElseStatement::accept(Visitor* visitor, std::unique_ptr<Statement>& node) {
	visitor->visit(this, node);
}

void IfElseStatement::generate() {
	llvm::Function* fun = g_builder->GetInsertBlock()->getParent();
	llvm::BasicBlock* mergeBlock = llvm::BasicBlock::Create(g_context, "mergeBlock");
	llvm::BasicBlock* currBlock = llvm::BasicBlock::Create(g_context, "ifBlock", fun);

	g_builder->CreateBr(currBlock);
	g_builder->SetInsertPoint(currBlock);

	for (size_t i = 0; i < m_conditions.size(); i++) {
		llvm::Value* condition = m_conditions[i]->generate();
		condition = llvm_utils::convertToBool(m_conditions[i]->getType(), condition);

		llvm::BasicBlock* nextBlock = nullptr;
		if (i != m_conditions.size() - 1) {
			nextBlock = llvm::BasicBlock::Create(g_context, "elifBlock");
		} else {
			if (m_conditions.size() < m_bodies.size()) {
				nextBlock = llvm::BasicBlock::Create(g_context, "elseBlock");
			} else {
				nextBlock = mergeBlock;
			}
		}

		llvm::BasicBlock* thisBlock = llvm::BasicBlock::Create(g_context, "thisBlock");
		g_builder->CreateCondBr(condition, thisBlock, nextBlock);

		fun->getBasicBlockList().push_back(thisBlock);
		g_builder->SetInsertPoint(thisBlock);

		g_module->addBlock();

		try {
			m_bodies[i]->generate();
			g_builder->CreateBr(mergeBlock);
		} catch (TerminatorAdded*) {}

		g_module->deleteBlock();

		fun->getBasicBlockList().push_back(nextBlock);
		g_builder->SetInsertPoint(nextBlock);
		currBlock = nextBlock;
	}

	if (m_conditions.size() < m_bodies.size()) { // has else statement
		g_module->addBlock();

		try {
			m_bodies.back()->generate();
			g_builder->CreateBr(mergeBlock);
		} catch (TerminatorAdded*) {}

		g_module->deleteBlock();

		fun->getBasicBlockList().push_back(mergeBlock);
		g_builder->SetInsertPoint(mergeBlock);
	}
}
