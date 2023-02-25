#include "DoWhileStatement.h"
#include <Parser/Visitor/Visitor.h>
#include <Module/Module.h>
#include <Module/LLVMUtils.h>
#include <Module/LLVMGlobals.h>
#include "../Cycles.h"

DoWhileStatement::DoWhileStatement(std::unique_ptr<Expression> condition, std::unique_ptr<Statement> body)
	: m_condition(std::move(condition)), m_body(std::move(body)) {

}

void DoWhileStatement::accept(Visitor* visitor, std::unique_ptr<Statement>& node) {
	visitor->visit(this, node);
}

void DoWhileStatement::generate() {
	llvm::Function* fun = g_builder->GetInsertBlock()->getParent();

	g_module->addBlock();

	llvm::BasicBlock* loopBB = llvm::BasicBlock::Create(g_context, "dowhile_body", fun);
	llvm::BasicBlock* condBB = llvm::BasicBlock::Create(g_context, "dowhile_condition");
	llvm::BasicBlock* endBB = llvm::BasicBlock::Create(g_context, "dowhile_end");
	g_cycles.addCycle({ condBB, endBB });
	g_builder->CreateBr(loopBB);
	g_builder->SetInsertPoint(loopBB);

	try {
		m_body->generate();
		g_builder->CreateBr(condBB);
	} catch (TerminatorAdded*) {}

	fun->getBasicBlockList().push_back(condBB);
	g_builder->SetInsertPoint(condBB);

	llvm::Value* condVal = m_condition->generate();
	condVal = llvm_utils::convertToBool(m_condition->getType(), condVal);
	g_builder->CreateCondBr(condVal, loopBB, endBB);

	fun->getBasicBlockList().push_back(endBB);
	g_builder->SetInsertPoint(endBB);

	g_cycles.deleteCycle();
	g_module->deleteBlock();
}

std::string DoWhileStatement::toString() const {
	std::string result = "do ";
	result += m_body->toString();
	while (result.back() == '\n') {
		result.pop_back();
	}

	if (result.back() != '}') {
		result += '\n';
	}

	result += "while ";
	result += m_condition->toString();
	result += ";\n";

	return result;
}
