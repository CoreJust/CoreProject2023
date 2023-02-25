#include "NopeStatement.h"
#include <Parser/Visitor/Visitor.h>

NopeStatement::NopeStatement() {

}

void NopeStatement::accept(Visitor* visitor, std::unique_ptr<Statement>& node) {
	visitor->visit(this, node);
}

void NopeStatement::generate() {

}

std::string NopeStatement::toString() const {
	return ";\n";
}
