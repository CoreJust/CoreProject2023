#include "SymbolTable.h"
#include <Utils/ErrorManager.h>

SymbolTable g_symbolTable;

void SymbolTable::addModuleSymbols(const std::string& path, ModuleSymbols symbols) {
	if (hasModuleSymbols(path)) {
		ErrorManager::internalError(ErrorID::E4051_LOADING_MODULE_SYMBOLS_TWICE, -1, "module name: " + path);
		return;
	}

	m_moduleSymbols[path] = std::move(symbols);
}

ModuleSymbols& SymbolTable::getModuleSymbols(const std::string& path) {
	return m_moduleSymbols[path];
}

bool SymbolTable::hasModuleSymbols(const std::string& path) const {
	return m_moduleSymbols.contains(path);
}