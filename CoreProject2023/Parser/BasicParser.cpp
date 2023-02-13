#include "BasicParser.h"
#include <Utils/ErrorManager.h>

static Token _NO_TOK = Token();


BasicParser::BasicParser(std::vector<Token>& tokens, u64& pos)
	: m_toks(tokens), m_pos(pos) {

}

Token& BasicParser::consume(TokenType type) {
	if (!match(type)) {
		ErrorManager::parserError(
			ErrorID::E2002_UNEXPECTED_TOKEN,
			getCurrLine(),
			"expected " + Token::toString(type)
		);
	}

	return peek(-1);
}

bool BasicParser::match(TokenType type) {
	if (peek().type != type) {
		return false;
	}

	m_pos++;
	return true;
}

bool BasicParser::matchRange(TokenType from, TokenType to) {
	TokenType type = peek().type;
	if (type < from || type > to) {
		return false;
	}

	m_pos++;
	return true;
}

Token& BasicParser::next() {
	if (m_pos >= m_toks.size()) {
		return _NO_TOK;
	}

	return m_toks[m_pos++];
}

Token& BasicParser::peek(int rel) {
	u64 pos = m_pos + rel;
	if (pos >= m_toks.size()) {
		return _NO_TOK;
	}

	return m_toks[pos];
}

int BasicParser::getCurrLine() {
	Token& tok = peek();
	if (tok.type == TokenType::NO_TOKEN) {
		return m_toks.back().errLine;
	}

	return tok.errLine;
}
