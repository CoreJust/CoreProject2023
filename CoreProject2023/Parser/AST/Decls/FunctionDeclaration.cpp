#include "FunctionDeclaration.h"
#include "llvm/IR/Verifier.h"
#include <Parser/Visitor/Visitor.h>
#include <Module/Module.h>
#include <Module/LLVMUtils.h>

Function* g_function;

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

		g_module->addBlock();
		g_function = m_function;
		size_t index = 0;
		for (size_t i = 0; i < m_function->prototype.args().size(); i++) {
			Argument& arg = m_function->prototype.args()[i];
			VariableQualities qualities;
			if (arg.type->isConst) {
				qualities.setVariableType(VariableType::CONST);
			}

			g_module->addLocalVariable(arg.name, arg.type->copy(), qualities,
				llvm_utils::genFunctionArgumentValue(m_function, arg, m_function->functionValue->getArg(i)));
		}
		
		try {
			m_body->generate();
			if (m_function->prototype.getReturnType()->basicType == BasicType::NO_TYPE) {
				g_builder->CreateRetVoid();
			}
		} catch (TerminatorAdded*) {}

		g_module->deleteBlock();
		llvm::verifyFunction(*fun);
		g_functionPassManager->run(*fun, *g_functionAnalysisManager);
	}
}