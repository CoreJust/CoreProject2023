#include "VariableDefStatement.h"
#include <Parser/Visitor/Visitor.h>
#include <Module/Module.h>
#include <Module/LLVMUtils.h>
#include <Module/LLVMGlobals.h>
#include <Utils/ErrorManager.h>

VariableDefStatement::VariableDefStatement(Variable var, std::unique_ptr<Expression> expr)
	: m_variable(std::move(var)), m_expr(std::move(expr)) {
	if (m_variable.type->safety == Safety::UNSAFE) {
		m_safety = Safety::UNSAFE;
		g_safety.tryUse(m_safety, m_errLine);
	}
}

void VariableDefStatement::accept(Visitor* visitor, std::unique_ptr<Statement>& node) {
	visitor->visit(this, node);
}

void VariableDefStatement::generate() {
	llvm::Value* alloc = nullptr;
	llvm::Function* fun = g_builder->GetInsertBlock()->getParent();
	if (m_variable.qualities.getVisibility() == Visibility::LOCAL) {
		llvm::Value* val;
		if (m_expr) {
			val = m_expr->generate();
			val = llvm_utils::tryImplicitlyConvertTo(
				m_variable.type,
				m_expr->getType(),
				val,
				m_errLine,
				m_expr->isCompileTime()
			);
		} else {
			val = llvm_utils::getDefaultValueOf(m_variable.type);
		}

		alloc = llvm_utils::createLocalVariable(fun, m_variable.type, m_variable.name);

		g_builder->CreateStore(val, alloc);
	} else {
		alloc = llvm_utils::createStaticVariable(fun, m_variable, m_expr.get());
	}

	g_module->addLocalVariable(
		m_variable.name,
		m_variable.type,
		m_variable.qualities,
		alloc
	);
}

std::string VariableDefStatement::toString() const {
	std::string result = m_variable.type->toString();
	result += ' ';
	result += m_variable.name;

	if (m_expr) {
		result += " = ";
		result += m_expr->toString();
	}

	result += ";\n";
	return result;
}
