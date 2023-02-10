#include "VariableDefStatement.h"
#include <Parser/Visitor/Visitor.h>
#include <Module/Module.h>
#include <Module/LLVMUtils.h>
#include <Utils/ErrorManager.h>

VariableDefStatement::VariableDefStatement(Variable var, std::unique_ptr<Expression> expr)
	: m_variable(std::move(var)), m_expr(std::move(expr)) {

}

void VariableDefStatement::accept(Visitor* visitor, std::unique_ptr<Statement>& node) {
	visitor->visit(this, node);
}

void VariableDefStatement::generate() {
	llvm::Value* val;
	if (m_expr) {
		if (m_variable.type->basicType == BasicType::REFERENCE) {
			val = m_expr->generateRValue();
		} else {
			val = m_expr->generate();
			val = llvm_utils::tryImplicitlyConvertTo(m_variable.type, m_expr->getType(), val, m_errLine, m_expr->isCompileTime());
		}
	} else {
		val = llvm_utils::getDefaultValueOf(m_variable.type);
	}

	llvm::Function* fun = g_builder->GetInsertBlock()->getParent();
	auto alloc = llvm_utils::createLocalVariable(fun, m_variable.type, m_variable.name);

	g_builder->CreateStore(val, alloc);
	g_module->addLocalVariable(m_variable.name, m_variable.type->copy(), m_variable.qualities, alloc);
}
