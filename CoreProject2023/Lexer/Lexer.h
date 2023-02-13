#pragma once
#include <vector>
#include <Utils/ErrorManager.h>
#include <Module/Symbols/Annotations.h>
#include "Token.h"

/*
	The class to convert the file's text to a vector of tokens, vector of imported files and file's module annotations
	The order:
		1. Creating an instance through the constructor.
		2. Calling handleModuleQualities() to read module annotations.
		3. Calling handleImports() to read the list of imports.
		4. Calling tokenize() to get a vector of tokens.
*/

class Lexer {
private:
	std::vector<Token> m_toks;
	const std::string& m_text;

	std::string m_buffer;
	u64 m_pos = 0;
	u64 m_nextLine = 0;
	u64 m_line = 0;

public:
	Lexer(const std::string& text);

	std::vector<Token> tokenize();

private:
	void skipModuleQualities();
	void skipImports();

private:
	void nextToken();

	void tokenizeNumber();
	void tokenizeOperator();
	void tokenizeText();
	void tokenizeCharacter();
	void tokenizeWord();

	// reads the line till the end and stores the read text to the last token
	void tokenizeLine();

	void tokenizeComment();

private:
	// reads a word and stores it to to
	void loadIdentifier(std::string &to);

	// reads a number and stores it to m_buffer in decimal format
	void loadNumber(int base, bool allowFloating, bool allowDelimiter);

	// returns a character in utf-32 format (since the current format is unknown)
	u32 getSingleChar();
	
	void skipWhitespaces(bool spacesOnly);
	void skipString();
	void printStringTranslationError(u32 errCode);

	int isKeyWord(std::string& s);
	bool isOperator(char ch);

	char next();
};