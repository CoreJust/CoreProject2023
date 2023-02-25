#include "ForStatement.h"
#include <Parser/Visitor/Visitor.h>
#include <Module/Module.h>
#include <Module/LLVMUtils.h>
#include <Module/LLVMGlobals.h>
#include "../Cycles.h"

ForStatement::ForStatement(
	std::vector<std::unique_ptr<Statement>> varDefs,
	std::unique_ptr<Expression> condition,
	std::vector<std::unique_ptr<Expression>> increments, 
	std::unique_ptr<Statement> body
) :
	m_varDefs(std::move(varDefs)),
	m_condition(std::move(condition)),
	m_increments(std::move(increments)),
	m_body(std::move(body)) {

}

void ForStatement::accept(Visitor* visitor, std::unique_ptr<Statement>& node) {
	visitor->visit(this, node);
}

void ForStatement::generate() {
	llvm::Function* fun = g_builder->GetInsertBlock()->getParent();

	g_module->addBlock();

	for (auto& var : m_varDefs) {
		var->generate();
	}

	llvm::BasicBlock* condBB = llvm::BasicBlock::Create(g_context, "for_condition", fun);
	llvm::BasicBlock* loopBB = llvm::BasicBlock::Create(g_context, "for_body");
	llvm::BasicBlock* incBB = llvm::BasicBlock::Create(g_context, "for_inc");
	llvm::BasicBlock* endBB = llvm::BasicBlock::Create(g_context, "for_end");
	g_cycles.addCycle({ incBB, endBB });
	g_builder->CreateBr(condBB);
	g_builder->SetInsertPoint(condBB);

	llvm::Value* condVal = m_condition->generate();
	condVal = llvm_utils::convertToBool(m_condition->getType(), condVal);
	g_builder->CreateCondBr(condVal, loopBB, endBB);

	fun->getBasicBlockList().push_back(loopBB);
	g_builder->SetInsertPoint(loopBB);

	try {
		m_body->generate();
		g_builder->CreateBr(incBB);
	} catch (TerminatorAdded*) {}

	fun->getBasicBlockList().push_back(incBB);
	g_builder->SetInsertPoint(incBB);

	for (auto& inc : m_increments) {
		inc->generate();
	}

	g_builder->CreateBr(condBB);

	fun->getBasicBlockList().push_back(endBB);
	g_builder->SetInsertPoint(endBB);

	g_cycles.deleteCycle();
	g_module->deleteBlock();
}
