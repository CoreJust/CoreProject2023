#include "FunctionDeclaration.h"
#include "llvm/IR/Verifier.h"
#include <Parser/Visitor/Visitor.h>

FunctionDeclaration::FunctionDeclaration(Function* func, std::unique_ptr<Statement> body)
	: m_function(func), m_body(std::move(body)) {

}

void FunctionDeclaration::accept(Visitor* visitor, std::unique_ptr<Declaration>& node) {
	visitor->visit(this, node);
}

void FunctionDeclaration::generate() {
	if (!m_function->qualities.isNative()) {
		llvm::Function* fun = m_function->functionValue;
		llvm::BasicBlock* bb = llvm::BasicBlock::Create(g_context, "entry", fun);
		g_builder->SetInsertPoint(bb);

		m_body->generate();

		llvm::verifyFunction(*fun);
		g_functionPassManager->run(*fun, *g_functionAnalysisManager);
	}
}