#include "ModuleSymbols.h"

void ModuleSymbolsUnit::addType(TypeNode type) {
	m_types.push_back(std::move(type));
}

void ModuleSymbolsUnit::addFunction(FunctionPrototype prototype, FunctionQualities qualities) {
	llvm::Function* funcVal = prototype.generate(qualities.isNative(), qualities.getCallingConvention());
	m_functions.push_back(Function{ std::move(prototype), qualities, funcVal });
}

void ModuleSymbolsUnit::addFunction(FunctionPrototype prototype, FunctionQualities qualities, llvm::Function* value) {
	m_functions.push_back(Function{ std::move(prototype), qualities, value });
}

void ModuleSymbolsUnit::addVariable(const std::string& name, std::unique_ptr<Type> type, VariableQualities qualities, llvm::Value* value) {
	m_variables.push_back(Variable{ name, std::move(type), qualities, value });
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

TypeNode* ModuleSymbolsUnit::getType(const std::string& name) {
	for (auto& type : m_types) {
		if (type.name == name) {
			return &type;
		}
	}

	return nullptr;
}

std::vector<Variable> ModuleSymbolsUnit::getVariables() {
	return m_variables;
}

std::vector<Function> ModuleSymbolsUnit::getFunctions() {
	return m_functions;
}
