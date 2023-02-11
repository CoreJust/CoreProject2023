#pragma once
#include <vector>
#include <memory>
#include <Lexer/Token.h>
#include "AST/Decls/Declaration.h"
#include "AST/States/Statement.h"
#include "AST/Exprs/Expression.h"

class Parser {
private:
	std::vector<Token> m_toks;
	u64 m_pos;

public:
	Parser(std::vector<Token> tokens);

	std::vector<std::unique_ptr<Declaration>> parse();

private:
	std::unique_ptr<Declaration> declaration();
	std::unique_ptr<Declaration> functionDeclaration();
	std::unique_ptr<Declaration> variableDeclaration();

	std::unique_ptr<Statement> stateOrBlock();
	std::unique_ptr<Statement> statement();
	std::unique_ptr<Statement> variableDefStatement();
	std::unique_ptr<Statement> whileStatement();
	std::unique_ptr<Statement> ifElseStatement();

	std::unique_ptr<Expression> expression();
	std::unique_ptr<Expression> assignment();
	std::unique_ptr<Expression> logical();
	std::unique_ptr<Expression> conditional();
	std::unique_ptr<Expression> rangeAndAs();
	std::unique_ptr<Expression> bitwise();
	std::unique_ptr<Expression> additive();
	std::unique_ptr<Expression> multiplicative();
	std::unique_ptr<Expression> degree();
	std::unique_ptr<Expression> unary();
	std::unique_ptr<Expression> postfix();
	std::unique_ptr<Expression> primary();

private:
	void skipAnnotation();

	Token& consume(TokenType type);
	bool match(TokenType type);
	bool matchRange(TokenType from, TokenType to);
	Token& next();
	Token& peek(int rel = 0);

	int getCurrLine();
};