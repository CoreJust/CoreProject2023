#pragma once
#include <vector>
#include <Lexer/Token.h>

class BasicParser {
protected:
	std::vector<Token>& m_toks;
	u64& m_pos;

public:
	BasicParser(std::vector<Token>& tokens, u64& pos);

protected:
	Token& consume(TokenType type);
	bool match(TokenType type);
	bool matchRange(TokenType from, TokenType to);
	Token& next();
	Token& peek(int rel = 0);

	int getCurrLine();
};
