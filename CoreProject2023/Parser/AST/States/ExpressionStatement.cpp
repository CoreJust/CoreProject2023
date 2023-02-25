#include "ExpressionStatement.h"
#include <Parser/Visitor/Visitor.h>

ExpressionStatement::ExpressionStatement(std::unique_ptr<Expression> expr)
	: m_expression(std::move(expr)) {

}

void ExpressionStatement::accept(Visitor* visitor, std::unique_ptr<Statement>& node) {
	visitor->visit(this, node);
}

void ExpressionStatement::generate() {
	m_expression->generate();
}

std::string ExpressionStatement::toString() const {
	return m_expression->toString() + ";\n";
}
