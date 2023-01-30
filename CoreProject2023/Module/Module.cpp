#include "Module.h"
#include <Utils/ErrorManager.h>
#include <Parser/AST/INode.h>

ModuleList g_moduleList;
ModuleRef g_module;

Module::Module(const std::string& name, const std::string& path, ModuleQualities qualities, std::vector<std::string> imports) 
	: m_name(name), m_path(path), m_qualities(qualities), m_importedModules(std::move(imports)),
		m_llvmModule(std::make_unique<llvm::Module>(name, g_context)) {

}

Module::Module(Module& other) 
	: Module(std::move(other)) {

}

void Module::loadSymbols() {
	// Loading this module's symbols
	auto& thisModule = g_symbolTable.getModuleSymbols(m_path);
	addModuleSymbolsUnit("", &thisModule.privateSymbols);
	addModuleSymbolsUnit("", &thisModule.publicOnceSymbols);
	addModuleSymbolsUnit("", &thisModule.publicSymbols);

	for (auto& importedModulePath : m_importedModules) {
		ModuleRef module = g_moduleList.getModule(importedModulePath);

		// Loading directly imported module's symbols
		auto& moduleSymbols = g_symbolTable.getModuleSymbols(importedModulePath);
		addModuleSymbolsUnit(module->getName(), &moduleSymbols.publicOnceSymbols);
		addModuleSymbolsUnit(module->getName(), &moduleSymbols.publicSymbols);
		m_allTheImportedModules[importedModulePath] = &moduleSymbols;

		// Loading indirectly imported module's symbols
		module->loadSymbolsIfNotLoaded();
		const auto& subimportedModules = module->getAllTheImportedModules();
		for (auto& subimportedModule : subimportedModules) {
			if (!m_allTheImportedModules.contains(subimportedModule.first)) {
				m_allTheImportedModules[subimportedModule.first] = subimportedModule.second;
				addModuleSymbolsUnit(
					g_moduleList.getModule(subimportedModule.first)->getName(),
					&subimportedModule.second->publicSymbols);
			}
		}
	}
}

void Module::addModuleAlias(const std::string& moduleName, const std::string& alias) {
	ASSERT(m_symbols.contains(moduleName), "cannot add alias if there is no such module");
	if (!m_symbols.contains(alias)) {
		m_symbols[alias] = m_symbols[moduleName];
	} else {
		auto& insertedSymbols = m_symbols[moduleName];
		m_symbols[alias].insert(m_symbols[alias].end(), insertedSymbols.begin(), insertedSymbols.end());
	}
}

SymbolType Module::getSymbolType(const std::string& name) const {
	if (m_symbols.contains(name)) {
		return SymbolType::MODULE;
	}

	for (ModuleSymbolsUnit* unit : m_symbols.at("")) {
		if (auto type = unit->getSymbolType(name); type != SymbolType::NO_SYMBOL) {
			return type;
		}
	}

	return SymbolType::NO_SYMBOL;
}

SymbolType Module::getSymbolType(const std::string& moduleAlias, const std::string& name) const {
	if (!m_symbols.contains(moduleAlias)) {
		ErrorManager::internalError(ErrorID::E4052_NO_MODULE_FOUND_BY_ALIAS, -1, "alias is: " + moduleAlias + ", symbol: " + name);
		return SymbolType::MODULE;
	}

	for (ModuleSymbolsUnit* unit : m_symbols.at(moduleAlias)) {
		if (auto type = unit->getSymbolType(name); type != SymbolType::NO_SYMBOL) {
			return type;
		}
	}

	return SymbolType::NO_SYMBOL;
}

Function* Module::getFunction(const std::string& name) {
	for (ModuleSymbolsUnit* unit : m_symbols.at("")) {
		if (auto fun = unit->getFunction(name); fun != nullptr) {
			return fun;
		}
	}

	return nullptr;
}

Function* Module::getFunction(const std::string& moduleAlias, const std::string& name) {
	for (ModuleSymbolsUnit* unit : m_symbols.at(moduleAlias)) {
		if (auto fun = unit->getFunction(name); fun != nullptr) {
			return fun;
		}
	}

	return nullptr;
}

Variable* Module::getVariable(const std::string& name) {
	for (ModuleSymbolsUnit* unit : m_symbols.at("")) {
		if (auto var = unit->getVariable(name); var != nullptr) {
			return var;
		}
	}

	return nullptr;
}

Variable* Module::getVariable(const std::string& moduleAlias, const std::string& name) {
	for (ModuleSymbolsUnit* unit : m_symbols.at(moduleAlias)) {
		if (auto var = unit->getVariable(name); var != nullptr) {
			return var;
		}
	}

	return nullptr;
}

const std::string& Module::getName() const noexcept {
	return m_name;
}

const std::string& Module::getPath() const noexcept {
	return m_path;
}

ModuleQualities Module::getQualities() const noexcept {
	return m_qualities;
}

const std::map<std::string, ModuleSymbols*>& Module::getAllTheImportedModules() const {
	return m_allTheImportedModules;
}

llvm::Module& Module::getLLVMModule() {
	return *m_llvmModule;
}

bool Module::areSymbolsLoaded() const {
	return !m_symbols.empty();
}

void Module::loadSymbolsIfNotLoaded() {
	if (!areSymbolsLoaded()) {
		loadSymbols();
	}
}

void Module::addModuleSymbolsUnit(const std::string& alias, ModuleSymbolsUnit* unit) {
	if (m_symbols.contains(alias)) {
		m_symbols[alias].push_back(unit);
	} else {
		m_symbols[alias] = { unit };
	}
}

std::string Module::getModuleNameFromPath(const std::string& path) {
	ASSERT(path.substr(path.size() - 5) == ".core", "Wrong path");

	size_t fileNameBeginning = path.find_last_of('/') + 1;
	size_t count = path.size() - fileNameBeginning - 5;
	return path.substr(fileNameBeginning, count);
}

std::string Module::getModulePathWithoutName(const std::string& path) {
	size_t fileNameBeginning = path.find_last_of('/') + 1;
	return path.substr(0, fileNameBeginning);
}


ModuleRef ModuleList::addModule(Module module) {
	m_modules.emplace_back(module);
	return ModuleRef(m_modules.size() - 1);
}

ModuleRef ModuleList::getModule(const std::string& path) {
	for (size_t i = 0; i < m_modules.size(); i++) {
		if (m_modules[i].getPath() == path) {
			return ModuleRef(i);
		}
	}

	return ModuleRef();
}

void ModuleList::setCurrentModule(const std::string& path) {
	if (auto mod = getModule(path); mod) {
		g_module = mod;
	} else {
		ErrorManager::internalError(ErrorID::E4053_CANNOT_SET_NO_MODULE_AS_CURRENT, -1, "path: " + path);
	}
}

std::vector<Module>& ModuleList::getModules() {
	return m_modules;
}

ModuleRef::ModuleRef(size_t i) 
	: index(i) {
	assert(g_moduleList.getModules().size() > i);
}

ModuleRef::ModuleRef()
	: index(std::string::npos) {

}

Module* ModuleRef::operator->() {
	return &g_moduleList.getModules()[index];
}

ModuleRef::operator bool() const {
	return index != std::string::npos;
}

Module& ModuleRef::operator*() {
	return g_moduleList.getModules()[index];
}
