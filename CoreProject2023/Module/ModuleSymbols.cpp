#include "ModuleSymbols.h"
#include <Module/LLVMGlobals.h>

void ModuleSymbolsUnit::addType(std::shared_ptr<TypeNode> type) {
	m_types.push_back(std::move(type));
}

void ModuleSymbolsUnit::addFunction(FunctionPrototype prototype) {
	m_functions.push_back(Function{ std::move(prototype), nullptr });
}

void ModuleSymbolsUnit::addConstructor(FunctionPrototype prototype) {
	m_constructors.push_back(Function{ std::move(prototype), nullptr });
}

void ModuleSymbolsUnit::addFunction(FunctionPrototype prototype, llvm::Function* value) {
	m_functions.push_back(Function{ std::move(prototype), value });
}

void ModuleSymbolsUnit::addConstructor(FunctionPrototype prototype, llvm::Function* value) {
	m_constructors.push_back(Function{ std::move(prototype), value });
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

	for (auto& fun : m_constructors) {
		if (fun.prototype.getName() == name) {
			return SymbolType::CONSTRUCTOR;
		}
	}

	for (auto& type : m_types) {
		if (type->name == name) {
			return SymbolType::TYPE;
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

				if (score == 0) {
					return result;
				}
			}
		}
	}

	return result;
}

Function* ModuleSymbolsUnit::chooseConstructor(
	const std::unique_ptr<Type>& type,
	const std::vector<std::unique_ptr<Type>>& argTypes,
	const std::vector<bool>& isCompileTime,
	bool isImlicit
) {
	Function* result = nullptr;
	i32 bestScore = -1;
	for (auto& fun : m_constructors) {
		if (fun.prototype.getReturnType()->equalsOrLessConstantThan(type) >= -4096
			&& fun.prototype.getQualities().isImplicit() >= isImlicit) {
			i32 score = fun.prototype.getSuitableness(argTypes, isCompileTime);
			if (score < 0) {
				continue;
			}

			if (result == nullptr || score < bestScore) {
				bestScore = score;
				result = &fun;

				if (score == 0) {
					return result;
				}
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

std::shared_ptr<TypeNode> ModuleSymbolsUnit::getType(const std::string& name) {
	for (auto& type : m_types) {
		if (type->name == name) {
			return type;
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

std::vector<Function>& ModuleSymbolsUnit::getConstructors() {
	return m_constructors;
}

std::vector<std::shared_ptr<TypeNode>>& ModuleSymbolsUnit::getTypes() {
	return m_types;
}

bool ModuleSymbolsUnit::isEmpty() const {
	return m_variables.size() == 0 && m_functions.size() == 0 && m_types.size() == 0;
}

ModuleSymbolsUnit& ModuleSymbols::getModuleSymbolsUnit(Visibility visibility) {
	switch (visibility) {
		case Visibility::PRIVATE: return privateSymbols;
		case Visibility::PUBLIC: return publicSymbols;
		case Visibility::DIRECT_IMPORT: return publicOnceSymbols;
	default:
		break;
	}

	ASSERT(false, "wrong visibility");
}

void ModuleSymbols::sortSymbolRefs() {
	std::sort(m_symbolRefs.begin(), m_symbolRefs.end(), [](const SymbolRef& a, const SymbolRef& b) -> bool {
		return a.tokenPos < b.tokenPos;
	});
}

void ModuleSymbols::addType(Visibility visibility, std::shared_ptr<TypeNode> type, u64 tokenPos) {
	ModuleSymbolsUnit& unit = getModuleSymbolsUnit(visibility);
	m_symbolRefs.push_back(SymbolRef{ tokenPos, unit.m_types.size(), SymbolType::TYPE, visibility });
	unit.addType(std::move(type));
}

void ModuleSymbols::addFunction(Visibility visibility, FunctionPrototype prototype, u64 tokenPos) {
	ModuleSymbolsUnit& unit = getModuleSymbolsUnit(visibility);
	m_symbolRefs.push_back(SymbolRef{ tokenPos, unit.m_functions.size(), SymbolType::FUNCTION, visibility });
	unit.addFunction(std::move(prototype));
}

void ModuleSymbols::addConstructor(Visibility visibility, FunctionPrototype prototype, u64 tokenPos) {
	ModuleSymbolsUnit& unit = getModuleSymbolsUnit(visibility);
	m_symbolRefs.push_back(SymbolRef{ tokenPos, unit.m_constructors.size(), SymbolType::CONSTRUCTOR, visibility });
	unit.addConstructor(std::move(prototype));
}

void ModuleSymbols::addVariable(Visibility visibility, const std::string& name, VariableQualities qualities, u64 tokenPos) {
	ModuleSymbolsUnit& unit = getModuleSymbolsUnit(visibility);
	m_symbolRefs.push_back(SymbolRef{ tokenPos, unit.m_variables.size(), SymbolType::VARIABLE, visibility });
	unit.addVariable(name, nullptr, qualities, nullptr);
}

Function* ModuleSymbols::getFunction(u64 tokenPos) {
	ASSERT(m_symbolRefs.size(), "No symbol refs");
	SymbolRef& symbol = getSymbolRefByTokenPos(0, m_symbolRefs.size() - 1, tokenPos);
	std::vector<Function>* funcsVec;

	if (symbol.symType == SymbolType::FUNCTION) {
		funcsVec = &getModuleSymbolsUnit(symbol.visibility).m_functions;
		ASSERT(symbol.index < funcsVec->size(), "functions: index out of range");
	} else if (symbol.symType == SymbolType::CONSTRUCTOR) {
		funcsVec = &getModuleSymbolsUnit(symbol.visibility).m_constructors;
		ASSERT(symbol.index < funcsVec->size(), "constructors: index out of range");
	} else {
		ASSERT(false, "not a constructor nor function");
	}

	return &(*funcsVec)[symbol.index];
}

Variable* ModuleSymbols::getVariable(u64 tokenPos) {
	ASSERT(m_symbolRefs.size(), "No symbol refs");
	SymbolRef& symbol = getSymbolRefByTokenPos(0, m_symbolRefs.size() - 1, tokenPos);
	auto& varsVec = getModuleSymbolsUnit(symbol.visibility).m_variables;

	ASSERT(symbol.symType == SymbolType::VARIABLE, "not a variable");
	ASSERT(symbol.index < varsVec.size(), "variables: index out of range");

	return &varsVec[symbol.index];
}

std::shared_ptr<TypeNode> ModuleSymbols::getType(u64 tokenPos) {
	ASSERT(m_symbolRefs.size(), "No symbol refs");
	SymbolRef& symbol = getSymbolRefByTokenPos(0, m_symbolRefs.size() - 1, tokenPos);
	auto& typesVec = getModuleSymbolsUnit(symbol.visibility).m_types;

	ASSERT(symbol.symType == SymbolType::TYPE, "not a type");
	ASSERT(symbol.index < typesVec.size(), "types: index out of range");

	return typesVec[symbol.index];
}

SymbolRef& ModuleSymbols::getSymbolRefByTokenPos(u64 from, u64 to, u64 tokenPos) {
	if (from == to) {
		SymbolRef& val = m_symbolRefs[from];
		if (val.tokenPos == tokenPos) {
			return val;
		} else {
			ASSERT(false, "no such value found");
		}
	}

	u64 middle = from + (to - from) / 2;
	SymbolRef& val = m_symbolRefs[middle];
	if (val.tokenPos == tokenPos) {
		return val;
	} else if (val.tokenPos < tokenPos) {
		return getSymbolRefByTokenPos(middle + 1, to, tokenPos);
	} else {
		return getSymbolRefByTokenPos(from, middle, tokenPos);
	}
}
