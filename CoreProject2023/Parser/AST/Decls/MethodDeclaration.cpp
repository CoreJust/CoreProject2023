#include "MethodDeclaration.h"
#include <llvm/IR/Verifier.h>
#include <Parser/Visitor/Visitor.h>
#include <Module/Module.h>
#include <Module/LLVMUtils.h>
#include <Module/LLVMGlobals.h>

extern std::shared_ptr<TypeNode> g_type;


MethodDeclaration::MethodDeclaration(Function* method, std::unique_ptr<Statement> body)
	: m_method(method), m_body(std::move(body)) {

}

void MethodDeclaration::accept(Visitor* visitor, std::unique_ptr<Declaration>& node) {
	visitor->visit(this, node);
}

void MethodDeclaration::generate() {
	if (!m_method->prototype.getQualities().isNative()) {
		llvm::Function* fun = m_method->functionManager->getOriginalValue();
		llvm::BasicBlock* bb = llvm::BasicBlock::Create(g_context, "entry", fun);
		g_builder->SetInsertPoint(bb);

		g_module->addBlock();

		g_function = m_method;
		size_t index = 0;
		for (size_t i = 0; i < m_method->prototype.args().size(); i++) {
			Argument& arg = m_method->prototype.args()[i];
			VariableQualities qualities;
			if (arg.type->isConst) {
				qualities.setVariableType(VariableType::CONST);
			}

			g_module->addLocalVariable(
				arg.name,
				arg.type->copy(),
				qualities,
				llvm_utils::genFunctionArgumentValue(
					m_method,
					arg,
					fun->getArg(i)
				)
			);
		}

		if (m_method->prototype.isUsingThisAsArgument()) {
			try {
				m_body->generate();
				if (m_method->prototype.getReturnType()->basicType == BasicType::NO_TYPE) {
					g_builder->CreateRetVoid();
				}
			}
			catch (TerminatorAdded*) {}
		} else {
			generateConstructor();
		}

		g_module->deleteBlock();
		llvm::verifyFunction(*fun);
		g_functionPassManager->run(*fun, *g_functionAnalysisManager);
	}
}

void MethodDeclaration::generateConstructor() {
	llvm::Function* fun = m_method->functionManager->getOriginalValue();
	llvm::Value* thisVar = llvm_utils::createLocalVariable(fun, m_method->prototype.getReturnType(), "this");

	g_module->addLocalVariable(
		"this",
		m_method->prototype.getReturnType()->copy(),
		VariableQualities(),
		thisVar
	);

	try {
		m_body->generate();
	} catch (TerminatorAdded*) {}

	thisVar = g_builder->CreateLoad(m_method->prototype.getReturnType()->to_llvm(), thisVar);
	g_builder->CreateRet(thisVar);
}
