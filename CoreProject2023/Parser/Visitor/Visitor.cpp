#include "Visitor.h"

void Visitor::visit(VariableDeclaration* decl, std::unique_ptr<Declaration>& node) {
	decl->m_value->accept(this, decl->m_value);
}

void Visitor::visit(FunctionDeclaration* decl, std::unique_ptr<Declaration>& node) {
	decl->m_body->accept(this, decl->m_body);
}

void Visitor::visit(BlockStatement* state, std::unique_ptr<Statement>& node) {
	for (auto& s : state->m_states) {
		s->accept(this, s);
	}
}

void Visitor::visit(WhileStatement* state, std::unique_ptr<Statement>& node) {
	state->m_condition->accept(this, state->m_condition);
	state->m_body->accept(this, state->m_body);
}

void Visitor::visit(IfElseStatement* state, std::unique_ptr<Statement>& node) {
	for (auto& s : state->m_bodies) {
		s->accept(this, s);
	}

	for (auto& s : state->m_conditions) {
		s->accept(this, s);
	}
}

void Visitor::visit(VariableDefStatement* state, std::unique_ptr<Statement>& node) {
	state->m_expr->accept(this, state->m_expr);
}

void Visitor::visit(ReturnStatement* state, std::unique_ptr<Statement>& node) {
	state->m_expression->accept(this, state->m_expression);
}

void Visitor::visit(ExpressionStatement* state, std::unique_ptr<Statement>& node) {
	state->m_expression->accept(this, state->m_expression);
}

void Visitor::visit(FunctionCallExpr* expr, std::unique_ptr<Expression>& node) {
	expr->m_funcExpr->accept(this, expr->m_funcExpr);
	for (auto& arg : expr->m_argExprs) {
		arg->accept(this, arg);
	}
}

void Visitor::visit(FunctionExpr* expr, std::unique_ptr<Expression>& node) {
	
}

void Visitor::visit(AssignmentExpr* expr, std::unique_ptr<Expression>& node) {
	expr->m_rval->accept(this, expr->m_rval);
	expr->m_expr->accept(this, expr->m_expr);
}

void Visitor::visit(BinaryExpr* expr, std::unique_ptr<Expression>& node) {
	expr->m_right->accept(this, expr->m_right);
	expr->m_left->accept(this, expr->m_left);
}

void Visitor::visit(UnaryExpr* expr, std::unique_ptr<Expression>& node) {
	expr->m_expr->accept(this, expr->m_expr);
}

void Visitor::visit(TypeConversionExpr* expr, std::unique_ptr<Expression>& node) {
	expr->m_expr->accept(this, expr->m_expr);
}

void Visitor::visit(VariableExpr* expr, std::unique_ptr<Expression>& node) {

}

void Visitor::visit(ValueExpr* expr, std::unique_ptr<Expression>& node) {
	
}
