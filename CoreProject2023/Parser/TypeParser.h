#pragma once
#include "BasicParser.h"
#include <Module/Symbols/Type.h>

// Note: do not handle TokenType::VAR
class TypeParser final : public BasicParser {
private:
	std::vector<u64> m_posHistory;

public:
	TypeParser(std::vector<Token>& toks, u64& pos);

	void skipConsumeType();
	bool skipType();

	// Requires a type, otherwise prints error
	std::unique_ptr<Type> consumeType();

	// Returns NO_TYPE if there is no type expression, returns m_pos to m_originalPos
	std::unique_ptr<Type> parseTypeOrGetNoType();

	// Returns NO_TYPE if there is no type expression, returns m_pos to m_originalPos
	std::unique_ptr<Type> tryParseType();

	// Returns nullptr if there is no type expression
	std::unique_ptr<Type> parseType();

	// isFalseOnDot - whether the -type-. is counted as not a type here
	bool isType(bool isFalseOnDot = false);

private:
	void savePos();
	void loadPos();
};