#include "WhileStatement.h"
#include <Parser/Visitor/Visitor.h>
#include <Module/Module.h>
#include <Module/LLVMUtils.h>
#include <Module/LLVMGlobals.h>
#include "../Cycles.h"

WhileStatement::WhileStatement(std::unique_ptr<Expression> condition, std::unique_ptr<Statement> body)
	: m_condition(std::move(condition)), m_body(std::move(body)) {

}

void WhileStatement::accept(Visitor* visitor, std::unique_ptr<Statement>& node) {
	visitor->visit(this, node);
}

void WhileStatement::generate() {
	llvm::Function* fun = g_builder->GetInsertBlock()->getParent();

	g_module->addBlock();

	llvm::BasicBlock* condBB = llvm::BasicBlock::Create(g_context, "while_condition", fun);
	llvm::BasicBlock* loopBB = llvm::BasicBlock::Create(g_context, "while_body");
	llvm::BasicBlock* endBB = llvm::BasicBlock::Create(g_context, "while_end");
	g_cycles.addCycle({ condBB, endBB });
	g_builder->CreateBr(condBB);
	g_builder->SetInsertPoint(condBB);

	llvm::Value* condVal = m_condition->generate();
	condVal = llvm_utils::convertToBool(m_condition->getType(), condVal);
	g_builder->CreateCondBr(condVal, loopBB, endBB);

	fun->getBasicBlockList().push_back(loopBB);
	g_builder->SetInsertPoint(loopBB);

	try {
		m_body->generate();
		g_builder->CreateBr(condBB);
	} catch (TerminatorAdded*) {}

	fun->getBasicBlockList().push_back(endBB);
	g_builder->SetInsertPoint(endBB);

	g_cycles.deleteCycle();
	g_module->deleteBlock();
}

std::string WhileStatement::toString() const {
	std::string result = "while ";
	result += m_condition->toString();
	result += " ";
	result += m_body->toString();

	return result;
}
