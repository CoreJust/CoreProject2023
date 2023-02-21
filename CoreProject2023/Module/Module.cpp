#include "Module.h"
#include <Utils/ErrorManager.h>
#include <Parser/AST/INode.h>
#include "LLVMUtils.h"
#include "LLVMGlobals.h"

ModuleList g_moduleList;
ModuleRef g_module;


Module::Module(
	const std::string& name, 
	const std::string& path, 
	ModuleQualities qualities, 
	std::vector<std::string> imports
) : 
	m_name(name),
	m_path(path), 
	m_qualities(qualities), 
	m_importedModules(std::move(imports)),
	m_llvmModule(std::make_unique<llvm::Module>(name, g_context)),
	m_ownSymbols(std::make_unique<ModuleSymbols>())
{
	m_symbols[""] = {
		&m_ownSymbols->privateSymbols,
		&m_ownSymbols->publicOnceSymbols,
		&m_ownSymbols->publicSymbols
	};
}

Module::Module(Module& other) 
	: Module(std::move(other)) {

}

void Module::loadImportsList() {
	if (m_allTheImportedModules.size()) {
		return;
	}

	m_allTheImportedModules.insert(m_path);
	for (auto& importedModulePath : m_importedModules) {
		ModuleRef module = g_moduleList.getModule(importedModulePath);
		m_allTheImportedModules.insert(module->getPath());
		m_symbols[module->getName()] = { };

		// Loading indirectly imported module's symbols
		module->loadImportsList();
		const auto& subimportedModules = module->getAllTheImportedModules();

		for (auto& subimportedModule : subimportedModules) {
			if (!m_allTheImportedModules.contains(subimportedModule)) {
				m_allTheImportedModules.insert(subimportedModule);
			}
		}
	}
}

void Module::loadSymbols() {
	if (m_areSymbolsLoaded) {
		return;
	} else {
		m_areSymbolsLoaded = true;
		m_allTheImportedModules.clear();
	}

	m_allTheImportedModules.insert(m_path);
	for (auto& importedModulePath : m_importedModules) {
		ModuleRef module = g_moduleList.getModule(importedModulePath);
		module->loadSymbols();

		// Loading directly imported module's symbols
		auto& moduleSymbols = module->getOwnSymbols();
		addModuleSymbolsUnit(module->getName(), &moduleSymbols.publicOnceSymbols);
		addModuleSymbolsUnit(module->getName(), &moduleSymbols.publicSymbols);
		m_allTheImportedModules.insert(module->getPath());

		// Loading indirectly imported module's symbols
		const auto& subimportedModules = module->getAllTheImportedModules();

		for (auto& subimportedModule : subimportedModules) {
			if (!m_allTheImportedModules.contains(subimportedModule)) {
				m_allTheImportedModules.insert(subimportedModule);
				addModuleSymbolsUnit(
					g_moduleList.getModule(subimportedModule)->getName(),
					&g_moduleList.getModule(subimportedModule)->getOwnSymbols().publicSymbols
				);
			}
		}
	}
}

void Module::loadAsLLVM() {
	loadThisModuleUnit(&m_ownSymbols->privateSymbols);
	loadThisModuleUnit(&m_ownSymbols->publicOnceSymbols);
	loadThisModuleUnit(&m_ownSymbols->publicSymbols);

	for (size_t i = 3; i < m_symbols[""].size(); i++) {
		loadModuleSymbolsAsLLVM(m_symbols[""][i]);
	}

	for (auto& units : m_symbols) {
		if (units.first != "") {
			for (auto& unit : units.second) {
				loadModuleSymbolsAsLLVM(unit);
			}
		}
	}
}

void Module::addAlias(
	SymbolType symType,
	const std::string& moduleName, 
	const std::string& name, 
	std::string alias
) {
	if (symType == SymbolType::MODULE) {
		if (!m_symbols.contains(alias)) {
			m_symbols[alias] = m_symbols[name];
		} else {
			auto& insertedSymbols = m_symbols[name];
			m_symbols[alias].insert(
				m_symbols[alias].end(),
				insertedSymbols.begin(),
				insertedSymbols.end()
			);
		}
	} else if (symType == SymbolType::VARIABLE) {
		Variable* var = getVariable(moduleName, name);
		if (alias.size() == 0) {
			alias = name;
		}

		m_symbols[""].back()->addVariable(alias, var->type->copy(), var->qualities, var->value);
	} else if (symType == SymbolType::FUNCTION) {
		Function* func = getFunction(moduleName, name);
		if (alias.size() == 0) {
			alias = name;
		}

		FunctionPrototype newProto = func->prototype;
		newProto.setName(alias);
		m_symbols[""].back()->addFunction(newProto, func->functionValue);
	}
}

void Module::addBlock() {
	m_localVariables.push_back(Variable{ "", nullptr, VariableQualities(), nullptr });
}

void Module::addLocalVariable(
	const std::string& name, 
	std::unique_ptr<Type> type, 
	VariableQualities qualities, 
	llvm::Value* value
) {
	m_localVariables.push_back(Variable{ name, std::move(type), qualities, value });
}

void Module::deleteBlock() {
	while (true) {
		if (m_localVariables.back().name == "") {
			m_localVariables.pop_back();
			break;
		}

		m_localVariables.pop_back();
	}
}

SymbolType Module::getSymbolType(const std::string& name) const {
	for (auto it = m_localVariables.rbegin(); it != m_localVariables.rend(); it++) {
		if (it->name == name) {
			return SymbolType::VARIABLE;
		}
	}

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
		ErrorManager::internalError(ErrorID::E4052_NO_MODULE_FOUND_BY_ALIAS, -1,
			"alias is: " + moduleAlias + ", symbol: " + name);

		return SymbolType::MODULE;
	}

	if (moduleAlias == "") {
		for (auto it = m_localVariables.rbegin(); it != m_localVariables.rend(); it++) {
			if (it->name == name) {
				return SymbolType::VARIABLE;
			}
		}
	}

	for (ModuleSymbolsUnit* unit : m_symbols.at(moduleAlias)) {
		if (auto type = unit->getSymbolType(name); type != SymbolType::NO_SYMBOL) {
			return type;
		}
	}

	return SymbolType::NO_SYMBOL;
}

Function* Module::getFunction(u64 tokenPos) {
	return m_ownSymbols->getFunction(tokenPos);
}

Function* Module::getFunction(const std::string& moduleAlias, const std::string& name) {
	Function* result = nullptr;
	for (ModuleSymbolsUnit* unit : m_symbols.at(moduleAlias)) {
		if (auto fun = unit->getFunction(name); fun != nullptr) {
			if (result != nullptr) {
				return nullptr;
			}

			result = fun;
		}
	}

	return result;
}

Function* Module::getFunction(
	const std::string& moduleName,
	const std::string& name,
	const std::vector<std::unique_ptr<Type>>& argTypes,
	const std::vector<bool>& isCompileTime
) {
	for (ModuleSymbolsUnit* unit : m_symbols.at(moduleName)) {
		if (auto fun = unit->getFunction(name, argTypes, isCompileTime); fun != nullptr) {
			return fun;
		}
	}

	return nullptr;
}

Function* Module::chooseFunction(
	const std::string& moduleName,
	const std::string& name,
	const std::vector<std::unique_ptr<Type>>& argTypes,
	const std::vector<bool>& isCompileTime
) {
	Function* result = nullptr;
	i32 bestScore = -1;

	for (ModuleSymbolsUnit* unit : m_symbols.at(moduleName)) {
		if (auto fun = unit->chooseFunction(name, argTypes, isCompileTime); fun != nullptr) {
			i32 score = fun->prototype.getSuitableness(argTypes, isCompileTime);
			if (result == nullptr || score < bestScore) {
				bestScore = score;
				result = fun;

				if (score == 0) {
					return fun;
				}
			}
		}
	}

	return result;
}

Function* Module::chooseConstructor(
	const std::unique_ptr<Type>& type,
	const std::vector<std::unique_ptr<Type>>& argTypes,
	const std::vector<bool>& isCompileTime,
	bool isImplicit
) {
	Function* result = nullptr;
	i32 bestScore = -1;

	for (auto& symbols : m_symbols) {
		for (ModuleSymbolsUnit* unit : symbols.second) {
			if (auto fun = unit->chooseConstructor(type, argTypes, isCompileTime, isImplicit); fun != nullptr) {
				i32 score = fun->prototype.getSuitableness(argTypes, isCompileTime);
				if (result == nullptr || score < bestScore) {
					bestScore = score;
					result = fun;

					if (score == 0) {
						return fun;
					}
				}
			}
		}
	}

	return result;
}

Function* Module::chooseOperator(
	const std::string& name,
	const std::vector<std::unique_ptr<Type>>& argTypes,
	const std::vector<bool>& isCompileTime,
	bool mustReturnReference
) {
	Function* result = nullptr;
	i32 bestScore = -1;

	for (auto& symbols : m_symbols) {
		for (ModuleSymbolsUnit* unit : symbols.second) {
			if (auto fun = unit->chooseOperator(name, argTypes, isCompileTime, mustReturnReference); fun != nullptr) {
				i32 score = fun->prototype.getSuitableness(argTypes, isCompileTime);
				if (result == nullptr || score < bestScore) {
					bestScore = score;
					result = fun;

					if (score == 0) {
						return fun;
					}
				}
			}
		}
	}

	return result;
}

Variable* Module::getVariable(u64 tokenPos) {
	return m_ownSymbols->getVariable(tokenPos);
}

Variable* Module::getVariable(const std::string& name) {
	return getVariable("", name);
}

Variable* Module::getVariable(const std::string& moduleAlias, const std::string& name) {
	if (moduleAlias == "") {
		for (auto it = m_localVariables.rbegin(); it != m_localVariables.rend(); it++) {
			if (it->name == name) {
				return &*it;
			}
		}
	}

	for (ModuleSymbolsUnit* unit : m_symbols.at(moduleAlias)) {
		if (auto var = unit->getVariable(name); var != nullptr) {
			return var;
		}
	}

	return nullptr;
}

std::shared_ptr<TypeNode> Module::getType(u64 tokenPos) {
	return m_ownSymbols->getType(tokenPos);
}

std::shared_ptr<TypeNode> Module::getType(const std::string& name) {
	return getType("", name);
}

std::shared_ptr<TypeNode> Module::getType(const std::string& moduleAlias, const std::string& name) {
	for (ModuleSymbolsUnit* unit : m_symbols.at(moduleAlias)) {
		if (auto type = unit->getType(name); type != nullptr) {
			return type;
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

const std::vector<std::string>& Module::getImports() const noexcept {
	return m_importedModules;
}

const std::set < std::string>& Module::getAllTheImportedModules() const {
	return m_allTheImportedModules;
}

ModuleSymbols& Module::getOwnSymbols() {
	return *m_ownSymbols;
}

llvm::Module& Module::getLLVMModule() {
	return *m_llvmModule;
}

void Module::loadThisModuleUnit(ModuleSymbolsUnit* unit) {
	// Addind to module's LLVM IR
	for (Function& func : unit->getFunctions()) {
		func.functionValue = func.prototype.generate();
	}

	for (Function& constructor : unit->getConstructors()) {
		constructor.functionValue = constructor.prototype.generate();
	}

	for (Function& op : unit->getOperators()) {
		op.functionValue = op.prototype.generate();
	}

	for (auto& type : unit->getTypes()) {
		for (Function& func : type->methods) {
			func.functionValue = func.prototype.generate();
		}
	}
}

void Module::addModuleSymbolsUnit(const std::string& alias, ModuleSymbolsUnit* unit) {
	if (unit->isEmpty()) {
		return;
	}

	// Adding to the module's symbols
	m_symbols[alias].push_back(unit);
}

void Module::loadModuleSymbolsAsLLVM(ModuleSymbolsUnit*& unit) {
	ModuleSymbolsUnit* newUnit = new ModuleSymbolsUnit();
	for (Variable& var : unit->getVariables()) {
		newUnit->addVariable(var.name, std::unique_ptr<Type>(var.type->copy()),
			var.qualities, llvm_utils::addGlobalVariableFromOtherModule(var, *m_llvmModule));
	}

	for (Function& func : unit->getFunctions()) {
		newUnit->addFunction(
			func.prototype,
			func.prototype.generateImportedFromOtherModule(*m_llvmModule)
		);
	}

	for (Function& constructor : unit->getConstructors()) {
		newUnit->addConstructor(
			constructor.prototype,
			constructor.prototype.generateImportedFromOtherModule(*m_llvmModule)
		);
	}

	for (Function& op : unit->getOperators()) {
		newUnit->addOperator(
			op.prototype,
			op.prototype.generateImportedFromOtherModule(*m_llvmModule)
		);
	}

	for (std::shared_ptr<TypeNode>& typeNode : unit->getTypes()) {
		std::shared_ptr<TypeNode> newTypeNode = std::make_shared<TypeNode>(
			typeNode->name,
			typeNode->qualities,
			typeNode->type->copy(),
			typeNode->llvmType,
			typeNode->fields,
			typeNode->methods,
			typeNode->internalTypes
		);

		for (Function& func : newTypeNode->methods) {
			func.functionValue = func.prototype.generateImportedFromOtherModule(*m_llvmModule);
		}
		
		for (Variable& var : newTypeNode->fields) {
			if (var.qualities.getVariableType() != VariableType::FIELD) {
				var.value = llvm_utils::addGlobalVariableFromOtherModule(var, *m_llvmModule);
			}
		}

		newUnit->addType(newTypeNode);
	}

	unit = newUnit;
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
