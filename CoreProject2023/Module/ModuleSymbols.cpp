#include "ModuleSymbols.h"

void ModuleSymbolsUnit::addFunction(std::unique_ptr<FunctionPrototype> prototype, FunctionQualities qualities) {
	m_functions.push_back(Function{ *prototype, qualities, prototype->generate(qualities.isNative()) });
}

void ModuleSymbolsUnit::addVariable(const std::string& name, VariableQualities qualities, llvm::Value* value) {
	m_variables.push_back(Variable{ name, qualities, value });
}

SymbolType ModuleSymbolsUnit::getSymbolType(const std::string& name) const {
	for (auto& var : m_variables) {
		if (var.name == name) {
			return SymbolType::VARIABLE;
		}
	}

	for (auto& fun : m_functions) {
		if (fun.prototype.getName() == name) {
			return SymbolType::FUNCTION;
		}
	}

	return SymbolType::NO_SYMBOL;
}

Function* ModuleSymbolsUnit::getFunction(const std::string& name) {
	for (auto& fun : m_functions) {
		if (fun.prototype.getName() == name) {
			return &fun;
		}
	}

	return nullptr;
}

Variable* ModuleSymbolsUnit::getVariable(const std::string& name) {
	for (auto& var : m_variables) {
		if (var.name == name) {
			return &var;
		}
	}

	return nullptr;
}
