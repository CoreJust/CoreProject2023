#pragma once
#include <optional>
#include "BasicSymbolLoader.h"
#include <Module/Module.h>

class SymbolLoader final : public BasicSymbolLoader {
public:
	SymbolLoader(
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
	std::optional<FunctionPrototype> loadMethod(TypeQualities parentQualities, std::shared_ptr<TypeNode> parentType);
	Variable loadField(TypeQualities parentQualities, std::shared_ptr<TypeNode> parentType);
};