#pragma once
#include <Parser/BasicParser.h>
#include <Module/Module.h>

class BasicSymbolLoader : public BasicParser {
protected:
	std::string m_path;
	ModuleSymbols& m_symbols;
	std::vector<std::vector<std::string>> m_annots;
	u64 m_truePos = 0;

public:
	BasicSymbolLoader(
		std::vector<Token>& toks,
		const std::string& path
	);

	void loadSymbols();

protected:
	virtual void loadUse() = 0;
	virtual void loadClass() = 0; // or another user-defined type
	virtual void loadFunction() = 0;
	virtual void loadVariable() = 0;
	virtual void loadTypeVariable() = 0;

	void skipCodeInBraces();
	void skipAssignment();
	void readAnnotations();
};