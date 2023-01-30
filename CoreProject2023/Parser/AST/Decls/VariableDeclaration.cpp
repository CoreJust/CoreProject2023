#include "VariableDeclaration.h"
#include <llvm/IR/GlobalVariable.h>
#include <llvm/Transforms/Utils/ModuleUtils.h>
#include <Parser/Visitor/Visitor.h>
#include <Module/Module.h>

VariableDeclaration::VariableDeclaration(Variable* var, std::unique_ptr<Expression> value) 
	: m_variable(var), m_value(std::move(value)) {

}

void VariableDeclaration::accept(Visitor* visitor, std::unique_ptr<Declaration>& node) {
	visitor->visit(this, node);
}

void VariableDeclaration::generate() {
	VariableType varType = m_variable->qualities.getVariableType();
	bool isConst = varType == VariableType::CONST;
	bool isExternal = varType == VariableType::EXTERN;

	llvm::Constant* defaultVal = llvm::ConstantInt::get(g_context, llvm::APInt(32, 0, true));
	llvm::GlobalVariable* var = new llvm::GlobalVariable(g_module->getLLVMModule(),
		llvm::Type::getInt32Ty(g_context), false, llvm::GlobalValue::LinkageTypes::ExternalLinkage,
		defaultVal, m_variable->name, nullptr, llvm::GlobalValue::NotThreadLocal, 0, false);

	m_variable->value = var;
	if (isExternal) {
		var->addAttribute("dso_local");
	}

	if (!isExternal && m_value) {
		llvm::FunctionType* initFuncType = llvm::FunctionType::get(llvm::Type::getVoidTy(g_context), false);
		llvm::Function* initFunc = llvm::Function::Create(initFuncType, llvm::Function::ExternalLinkage,
			"$init_" + m_variable->name, g_module->getLLVMModule());
		llvm::BasicBlock* bb = llvm::BasicBlock::Create(g_context, "entry", initFunc);
		g_builder->SetInsertPoint(bb);
		
		llvm::Value* val = m_value->generate();
		g_builder->CreateStore(val, var, m_variable->qualities.getVisibility() != Visibility::PRIVATE);
		g_builder->CreateRetVoid();

		llvm::appendToGlobalCtors(g_module->getLLVMModule(), initFunc, 1);
	}
}
