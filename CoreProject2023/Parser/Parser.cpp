#include "Parser.h"
#include <Utils/ErrorManager.h>
#include "AST/AST.h"

static Token _NO_TOK = Token();

Parser::Parser(std::vector<Token> tokens) : m_toks(std::move(tokens)), m_pos(0) { }

std::unique_ptr<Expression> Parser::parse() {
	return primary();
}

std::unique_ptr<Expression> Parser::primary() {
	if (match(TokenType::NUMBERI32)) {
		return std::make_unique<ValueExpr>(std::stol(peek(-1).data));
	}

	ErrorManager::parserError(getCurrLine(), "No expression found");
}

Token& Parser::consume(TokenType type) {
	if (!match(type))
		ErrorManager::parserError(getCurrLine(), "expected " + Token::toString(type));

	return peek(-1);
}

bool Parser::match(TokenType type) {
	if (peek().type != type)
		return false;

	m_pos++;
	return true;
}

bool Parser::matchRange(TokenType from, TokenType to) {
	auto type = peek().type;
	if (type < from || type > to)
		return false;

	m_pos++;
	return true;
}

Token& Parser::next() {
	if (m_pos >= m_toks.size())
		return _NO_TOK;

	return m_toks[m_pos++];
}

Token& Parser::peek(int rel) {
	u64 pos = m_pos + rel;
	if (pos >= m_toks.size())
		return _NO_TOK;

	return m_toks[pos];
}

int Parser::getCurrLine() {
	auto tok = peek();
	if (tok.type == TokenType::NO_TOKEN)
		return m_toks.back().errLine;

	return tok.errLine;
}