#include "BlockStatement.h"
#include <Parser/Visitor/Visitor.h>
#include <Module/Module.h>

BlockStatement::BlockStatement(std::vector<std::unique_ptr<Statement>> states) 
	: m_states(std::move(states)) {

}

void BlockStatement::accept(Visitor* visitor, std::unique_ptr<Statement>& node) {
	visitor->visit(this, node);
}

void BlockStatement::generate() {
	g_module->addBlock();
	for (auto& state : m_states) {
		state->generate();
	}

	g_module->deleteBlock();
}

std::string BlockStatement::toString() const {
	static std::string s_tabs = "";

	s_tabs += '\t';
	std::string result = "{\n";
	for (auto& state : m_states) {
		result += s_tabs;
		result += state->toString();
	}

	result += "}\n\n";
	s_tabs.pop_back();

	return result;
}
