#pragma once
#include <memory>
#include <Parser/AST/AST.h>

class Visitor {
public:
	virtual void visit(TypeDeclaration* decl, std::unique_ptr<Declaration>& node);
	virtual void visit(MethodDeclaration* decl, std::unique_ptr<Declaration>& node);
	virtual void visit(FieldDeclaration* decl, std::unique_ptr<Declaration>& node);
	virtual void visit(VariableDeclaration* decl, std::unique_ptr<Declaration>& node);
	virtual void visit(FunctionDeclaration* decl, std::unique_ptr<Declaration>& node);

	virtual void visit(BlockStatement* state, std::unique_ptr<Statement>& node);
	virtual void visit(ForStatement* state, std::unique_ptr<Statement>& node);
	virtual void visit(WhileStatement* state, std::unique_ptr<Statement>& node);
	virtual void visit(DoWhileStatement* state, std::unique_ptr<Statement>& node);
	virtual void visit(IfElseStatement* state, std::unique_ptr<Statement>& node);
	virtual void visit(VariableDefStatement* state, std::unique_ptr<Statement>& node);
	virtual void visit(ReturnStatement* state, std::unique_ptr<Statement>& node);
	virtual void visit(ExpressionStatement* state, std::unique_ptr<Statement>& node);
	virtual void visit(NopeStatement* state, std::unique_ptr<Statement>& node);

	virtual void visit(MethodCallExpr* expr, std::unique_ptr<Expression>& node);
	virtual void visit(FunctionCallExpr* expr, std::unique_ptr<Expression>& node);
	virtual void visit(FunctionExpr* expr, std::unique_ptr<Expression>& node);
	virtual void visit(AssignmentExpr* expr, std::unique_ptr<Expression>& node);
	virtual void visit(ConditionalExpr* expr, std::unique_ptr<Expression>& node);
	virtual void visit(BinaryExpr* expr, std::unique_ptr<Expression>& node);
	virtual void visit(UnaryExpr* expr, std::unique_ptr<Expression>& node);
	virtual void visit(ArrayElementAccessExpr* expr, std::unique_ptr<Expression>& node);
	virtual void visit(FieldAccessExpr* expr, std::unique_ptr<Expression>& node);
	virtual void visit(TypeConversionExpr* expr, std::unique_ptr<Expression>& node);
	virtual void visit(VariableExpr* expr, std::unique_ptr<Expression>& node);
	virtual void visit(ArrayExpr* expr, std::unique_ptr<Expression>& node);
	virtual void visit(ValueExpr* expr, std::unique_ptr<Expression>& node);
};