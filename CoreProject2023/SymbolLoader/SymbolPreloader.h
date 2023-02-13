#pragma once
#include "BasicSymbolLoader.h"
#include <Module/Module.h>

class SymbolPreloader final : public BasicSymbolLoader {
public:
	SymbolPreloader(
		std::vector<Token>& toks,
		const std::string& path
	);

private:
	void loadUse();
	void loadClass(); // or another user-defined type
	void loadFunction();
	void loadVariable();
	void loadTypeVariable();
};