#pragma once
#include <Lexer/Token.h>
#include <Module/SymbolTable.h>
#include <Module/Module.h>

class SymbolLoader final {
private:
	std::string m_path;
	ModuleSymbols m_symbols;
	ModuleQualities m_qualities;
	std::vector<Token>& m_toks;
	std::vector<std::vector<std::string>> m_annots;
	size_t m_pos = 0;

public:
	SymbolLoader(std::vector<Token>& toks, ModuleQualities qualities, const std::string& path);

	void loadSymbols();

private:
	void loadUse();
	void loadClass(); // or another user-defined type
	void loadFunction();
	void loadVariable();

	void skipCodeInBraces();
	void skipAssignment();
	void readAnnotations();

	bool match(TokenType type);
	void consume(TokenType type);
	Token& peek(int rel = 0);
	int getCurrLine();
};