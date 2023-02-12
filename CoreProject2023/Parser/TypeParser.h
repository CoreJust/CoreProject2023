#pragma once
#include <Lexer/Token.h>
#include <Module/Symbols/Type.h>

// Note: do not handle TokenType::VAR
class TypeParser {
private:
	std::vector<Token>& m_toks;
	u64& m_pos;
	std::vector<u64> m_posHistory;

public:
	TypeParser(std::vector<Token>& toks, u64& pos);

	// requires a type, otherwise prints error
	std::unique_ptr<Type> consumeType();

	// returns NO_TYPE if there is no type expression, returns m_pos to m_originalPos
	std::unique_ptr<Type> parseTypeOrGetNoType();

	// returns NO_TYPE if there is no type expression, returns m_pos to m_originalPos
	std::unique_ptr<Type> tryParseType();

	// returns nullptr if there is no type expression
	std::unique_ptr<Type> parseType();

	bool isType();

private:
	Token& consume(TokenType type);
	bool match(TokenType type);
	bool matchRange(TokenType from, TokenType to);
	Token& next();
	Token& peek(int rel = 0);
	int getCurrLine();

	void savePos();
	void loadPos();
};