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
	void loadUse() override;
	void loadClass() override; // or another user-defined type
	void loadFunction() override;
	void loadVariable() override;
	void loadTypeVariable() override;

private:
	void loadMethod(TypeQualities parentQualities);
};