#include "VariableDeclaration.h"
#include <Parser/Visitor/Visitor.h>
#include <Module/LLVMUtils.h>
#include <Module/LLVMGlobals.h>

VariableDeclaration::VariableDeclaration(Variable* var, std::unique_ptr<Expression> value) 
	: m_variable(var), m_value(std::move(value)) {
	m_safety = m_variable->qualities.getSafety();
	if (m_variable->type->safety == Safety::UNSAFE) {
		g_safety.tryUse(m_safety, Safety::UNSAFE, m_errLine);
	}
}

void VariableDeclaration::accept(Visitor* visitor, std::unique_ptr<Declaration>& node) {
	visitor->visit(this, node);
}

void VariableDeclaration::generate() {
	g_safety.push(m_safety);
	llvm_utils::createGlobalVariable(*m_variable, m_value.get());
	g_safety.pop();
}

std::string VariableDeclaration::toString() const {
	static std::string VISIBILITY_STR[4] = { "@local\n", "@private\n", "@direct_import\n", "@public\n" };
	static std::string SAFETY_STR[3] = { "@unsafe\n", "@safe_only\n", "@safe\n" };

	std::string result = "";
	if (m_variable->qualities.isThreadLocal()) {
		result += "@thread_local\n";
	}

	result += SAFETY_STR[(u8)m_variable->qualities.getSafety()];
	result += VISIBILITY_STR[(u8)m_variable->qualities.getVisibility()];

	result += m_variable->type->toString();
	result += ' ';
	result += m_variable->name;

	if (m_value) {
		result += " = ";
		result += m_value->toString();
	}

	result += ";\n";
	return result;
}
