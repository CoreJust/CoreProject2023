#pragma once
#include <vector>
#include <llvm/IR/Function.h>
#include "FunctionArgument.h"

class FunctionPrototype final {
public:
	FunctionPrototype(const std::string& name, std::vector<Argument> args);

	llvm::Function* generate(bool is_native) const;

	const std::string& getName() const;
	void setName(const std::string& s);

	std::vector<Argument>& args();

private:
	std::string m_name;
	std::vector<Argument> m_args;
};