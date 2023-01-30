#include "BlockStatement.h"
#include <Parser/Visitor/Visitor.h>

BlockStatement::BlockStatement(std::vector<std::unique_ptr<Statement>> states) 
	: m_states(std::move(states)) {

}

void BlockStatement::accept(Visitor* visitor, std::unique_ptr<Statement>& node) {
	visitor->visit(this, node);
}

void BlockStatement::generate() {
	for (auto& state : m_states) {
		state->generate();
	}
}
