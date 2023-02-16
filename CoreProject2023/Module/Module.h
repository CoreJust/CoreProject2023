#pragma once
#include <llvm/IR/Module.h>
#include "ModuleSymbols.h"

class Module final {
private:
	std::string m_name;
	std::string m_path;
	ModuleQualities m_qualities;
	std::unique_ptr<llvm::Module> m_llvmModule;
	std::vector<std::string> m_importedModules;

	// key "" means accessible without stating any module/namespace
	std::map<std::string, std::vector<ModuleSymbolsUnit*>> m_symbols;
	std::unique_ptr<ModuleSymbols> m_ownSymbols;

	// needed to get symbols from imports in imported modules
	std::set<std::string> m_allTheImportedModules; 

	std::vector<Variable> m_localVariables;

	bool m_areSymbolsLoaded = false;

public:
	Module(
		const std::string& name,
		const std::string& path, 
		ModuleQualities qualities, 
		std::vector<std::string> imports
	);

	Module(Module& other);
	Module(Module&& other) = default;

	// makes a list of all the imported modules
	void loadImportsList();
	void loadSymbols();
	void loadAsLLVM(); // loads symbols to llvm::Module

	void addAlias(
		SymbolType symType,
		const std::string& moduleName,
		const std::string& name, 
		std::string alias
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

	Function* getFunction(u64 tokenPos); // as well as constructor

	// Tries to get a function by name
	// Returns nullptr if nothing found or more than one function with such name exist
	Function* getFunction(const std::string& moduleName, const std::string& name);

	// Finds the function with the name and exactly argTypes
	Function* getFunction(
		const std::string& moduleName,
		const std::string& name,
		const std::vector<std::unique_ptr<Type>>& argTypes,
		const std::vector<bool>& isCompileTime
	);

	// Chooses the most suitable function with name for argTypes
	Function* chooseFunction(
		const std::string& moduleName,
		const std::string& name,
		const std::vector<std::unique_ptr<Type>>& argTypes,
		const std::vector<bool>& isCompileTime
	);

	Function* chooseConstructor(
		const std::unique_ptr<Type>& type,
		const std::vector<std::unique_ptr<Type>>& argTypes,
		const std::vector<bool>& isCompileTime,
		bool isImplicit
	);

	Variable* getVariable(u64 tokenPos);
	Variable* getVariable(const std::string& name);
	Variable* getVariable(const std::string& moduleAlias, const std::string& name);

	std::shared_ptr<TypeNode> getType(u64 tokenPos);
	std::shared_ptr<TypeNode> getType(const std::string& name);
	std::shared_ptr<TypeNode> getType(const std::string& moduleAlias, const std::string& name);

	const std::string& getName() const noexcept;
	const std::string& getPath() const noexcept;
	ModuleQualities getQualities() const noexcept;

	const std::vector<std::string>& getImports() const noexcept;
	const std::set<std::string>& getAllTheImportedModules() const;

	ModuleSymbols& getOwnSymbols();
	llvm::Module& getLLVMModule();

private:
	void loadThisModuleUnit(ModuleSymbolsUnit* unit);
	void addModuleSymbolsUnit(const std::string& alias, ModuleSymbolsUnit* unit);
	void loadModuleSymbolsAsLLVM(ModuleSymbolsUnit*& unit);

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