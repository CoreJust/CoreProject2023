#pragma once
#include <vector>
#include <memory>
#include <Lexer/Token.h>
#include "AST/Expression.h"

class Parser {
private:
	std::vector<Token> m_toks;
	u64 m_pos;

public:
	Parser(std::vector<Token> tokens);

	std::unique_ptr<Expression> parse();

private:
	std::unique_ptr<Expression> primary();

private:
	Token& consume(TokenType type);
	bool match(TokenType type);
	bool matchRange(TokenType from, TokenType to);
	Token& next();
	Token& peek(int rel = 0);
	int getCurrLine();
};