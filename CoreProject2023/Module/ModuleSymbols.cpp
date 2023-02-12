#include "ModuleSymbols.h"
#include <Module/LLVMGlobals.h>

void ModuleSymbolsUnit::addType(TypeNode type) {
	m_types.push_back(std::move(type));
}

void ModuleSymbolsUnit::addFunction(FunctionPrototype prototype) {
	llvm::Function* funcVal = prototype.generate();
	m_functions.push_back(Function{ std::move(prototype), funcVal });
}

void ModuleSymbolsUnit::addFunction(FunctionPrototype prototype, llvm::Function* value) {
	m_functions.push_back(Function{ std::move(prototype), value });
}

void ModuleSymbolsUnit::addVariable(
	const std::string& name, 
	std::unique_ptr<Type> type, 
	VariableQualities qualities, 
	llvm::Value* value
) {
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
	Function* result = nullptr;
	for (auto& fun : m_functions) {
		if (fun.prototype.getName() == name) {
			if (result != nullptr) {
				return nullptr;
			}

			result = &fun;
		}
	}

	return result;
}

Function* ModuleSymbolsUnit::getFunction(
	const std::string& name,
	const std::vector<std::unique_ptr<Type>>& argTypes,
	const std::vector<bool>& isCompileTime
) {
	for (auto& fun : m_functions) {
		if (fun.prototype.getName() == name) {
			i32 score = fun.prototype.getSuitableness(argTypes, isCompileTime);
			if (score == 0) {
				return &fun;
			}
		}
	}

	return nullptr;
}

Function* ModuleSymbolsUnit::chooseFunction(
	const std::string& name,
	const std::vector<std::unique_ptr<Type>>& argTypes,
	const std::vector<bool>& isCompileTime
) {
	Function* result = nullptr;
	i32 bestScore = -1;
	for (auto& fun : m_functions) {
		if (fun.prototype.getName() == name) {
			i32 score = fun.prototype.getSuitableness(argTypes, isCompileTime);
			if (score < 0) {
				continue;
			}

			if (result == nullptr || score < bestScore) {
				bestScore = score;
				result = &fun;
			}
		}
	}

	return result;
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

std::vector<Variable>& ModuleSymbolsUnit::getVariables() {
	return m_variables;
}

std::vector<Function>& ModuleSymbolsUnit::getFunctions() {
	return m_functions;
}

bool ModuleSymbolsUnit::isEmpty() const {
	return m_variables.size() == 0 && m_functions.size() == 0 && m_types.size() == 0;
}
