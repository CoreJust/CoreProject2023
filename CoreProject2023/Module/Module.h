#pragma once
#include <llvm/IR/Module.h>
#include "SymbolTable.h"

class Module final {
private:
	std::string m_name;
	std::string m_path;
	ModuleQualities m_qualities;
	std::unique_ptr<llvm::Module> m_llvmModule;
	std::vector<std::string> m_importedModules;

	// key "" means accessible without stating any module/namespace
	std::map<std::string, std::vector<ModuleSymbolsUnit*>> m_symbols;

	// needed to get symbols from imports in imported modules
	std::map<std::string, ModuleSymbols*> m_allTheImportedModules; 

	std::vector<Variable> m_localVariables;

public:
	Module(
		const std::string& name,
		const std::string& path, 
		ModuleQualities qualities, 
		std::vector<std::string> imports
	);

	Module(Module& other);
	Module(Module&& other) = default;

	void loadSymbols();
	void addModuleAlias(
		const std::string& moduleName, 
		const std::string& alias
	);

	void addBlock();
	void addLocalVariable(
		const std::string& name, 
		std::unique_ptr<Type> type, 
		VariableQualities qualities, 
		llvm::Value* value
	);

	void deleteBlock();

	SymbolType getSymbolType(const std::string& name) const;
	SymbolType getSymbolType(const std::string& moduleAlias, const std::string& name) const;

	Function* getFunction(const std::string& name);
	Function* getFunction(const std::string& moduleAlias, const std::string& name);
	Variable* getVariable(const std::string& name);
	Variable* getVariable(const std::string& moduleAlias, const std::string& name);
	TypeNode* getType(const std::string& name);
	TypeNode* getType(const std::string& moduleAlias, const std::string& name);

	const std::string& getName() const noexcept;
	const std::string& getPath() const noexcept;
	ModuleQualities getQualities() const noexcept;
	const std::map<std::string, ModuleSymbols*>& getAllTheImportedModules() const;
	llvm::Module& getLLVMModule();

	bool areSymbolsLoaded() const;

private:
	void loadSymbolsIfNotLoaded();
	void addModuleSymbolsUnit(const std::string& alias, ModuleSymbolsUnit* unit);

public:
	static std::string getModuleNameFromPath(const std::string& path);
	static std::string getModulePathWithoutName(const std::string& path);
};


// This class is used to replace Module* so that it wouldn't break once g_modules' memory is reallocated
class ModuleRef final {
public:
	size_t index;

public:
	ModuleRef(size_t i);
	ModuleRef();

	Module* operator->();
	Module& operator*();
	explicit operator bool() const;
};


class ModuleList final {
private:
	std::vector<Module> m_modules;

public:
	ModuleRef addModule(Module module);
	ModuleRef getModule(const std::string& path);
	void setCurrentModule(const std::string& path);

	std::vector<Module>& getModules();
};

extern ModuleList g_moduleList;
extern ModuleRef g_module;