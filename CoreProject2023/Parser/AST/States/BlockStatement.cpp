#include "BlockStatement.h"
#include <Parser/Visitor/Visitor.h>
#include <Module/Module.h>

BlockStatement::BlockStatement(std::vector<std::unique_ptr<Statement>> states, Safety safety) 
	: m_states(std::move(states)) {
	m_safety = safety;
}

void BlockStatement::accept(Visitor* visitor, std::unique_ptr<Statement>& node) {
	visitor->visit(this, node);
}

void BlockStatement::generate() {
	g_module->addBlock();
	g_safety.push(m_safety);
	for (auto& state : m_states) {
		state->generate();
	}

	g_safety.pop();
	g_module->deleteBlock();
}

std::string BlockStatement::toString() const {
	std::string result = "\n" + s_tabs + "{\n";

	s_tabs += '\t';
	for (auto& state : m_states) {
		result += s_tabs;
		result += state->toString();
	}

	s_tabs.pop_back();
	result += s_tabs;
	result += "}\n";

	return result;
}
