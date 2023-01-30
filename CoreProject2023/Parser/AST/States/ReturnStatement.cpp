#include "ReturnStatement.h"
#include <Parser/Visitor/Visitor.h>

ReturnStatement::ReturnStatement(std::unique_ptr<Expression> expr) 
	: m_expression(std::move(expr)) {

}

void ReturnStatement::accept(Visitor* visitor, std::unique_ptr<Statement>& node) {
	visitor->visit(this, node);
}

void ReturnStatement::generate() {
	if (m_expression) {
		llvm::Value* value = m_expression->generate();
		g_builder->CreateRet(value);
	} else {
		g_builder->CreateRetVoid();
	}
}
