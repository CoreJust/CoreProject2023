#pragma once
#include <map>
#include "ModuleSymbols.h"

class SymbolTable final {
private:
	std::map<std::string, ModuleSymbols> m_moduleSymbols;

public:
	void addModuleSymbols(const std::string &path, ModuleSymbols symbols);
	ModuleSymbols& getModuleSymbols(const std::string& path);
	bool hasModuleSymbols(const std::string& path) const;
};

extern SymbolTable g_symbolTable;