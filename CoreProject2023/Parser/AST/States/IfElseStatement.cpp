#include "IfElseStatement.h"
#include <Parser/Visitor/Visitor.h>
#include <Module/Module.h>
#include <Module/LLVMUtils.h>
#include <Module/LLVMGlobals.h>

IfElseStatement::IfElseStatement(
	std::vector<std::unique_ptr<Statement>> bodies,
	std::vector<std::unique_ptr<Expression>> conditions
) : 
	m_bodies(std::move(bodies)),
	m_conditions(std::move(conditions)) 
{
	ASSERT(m_conditions.size() && m_conditions.size() <= m_bodies.size(), "Wrong if statement");
}

void IfElseStatement::accept(Visitor* visitor, std::unique_ptr<Statement>& node) {
	visitor->visit(this, node);
}

void IfElseStatement::generate() {
	llvm::Function* fun = g_builder->GetInsertBlock()->getParent();
	llvm::BasicBlock* mergeBlock = llvm::BasicBlock::Create(g_context, "mergeBlock");
	llvm::BasicBlock* currBlock = llvm::BasicBlock::Create(g_context, "ifBlock", fun);
	bool addMergeBlock = false;

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
			addMergeBlock = true;
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
			addMergeBlock = true;
		} catch (TerminatorAdded*) {}

		g_module->deleteBlock();

		if (addMergeBlock) {
			fun->getBasicBlockList().push_back(mergeBlock);
			g_builder->SetInsertPoint(mergeBlock);
		}
	}
}

std::string IfElseStatement::toString() const {
	std::string result = "";
	size_t i = 0;
	for (; i < m_conditions.size(); i++) {
		result += "if ";
		result += m_conditions[i]->toString();
		result += " ";
		result += m_bodies[i]->toString();

		if (result.back() == '}') {
			result += " else ";
		} else {
			result += "\nelse ";
		}
	}

	if (m_bodies.size() > i) {
		result += m_bodies[i]->toString();
	} else {
		result.erase(result.size() - 6);
	}

	return result;
}
