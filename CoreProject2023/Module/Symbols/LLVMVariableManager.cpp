#include "LLVMVariableManager.h"
#include "Variable.h"
#include <Project/Project.h>
#include <Module/Module.h>
#include <Module/LLVMUtils.h>

void LLVMVariableManager::setInitialValue(llvm::Value* varValue) {
	m_variableValues[g_module->getPath()] = varValue;
	m_originalValue = varValue;
}

llvm::Value* LLVMVariableManager::getVariableValueForCurrentModule(Variable* var) {
	if (g_settings->compilationMode == CompilationMode::Program) {
		return m_originalValue;
	}

	if (!m_variableValues.contains(g_module->getPath())) {
		m_variableValues[g_module->getPath()] = llvm_utils::addGlobalVariableFromOtherModule(*var, g_module->getLLVMModule());
	}

	return m_variableValues[g_module->getPath()];
}

llvm::Value* LLVMVariableManager::getOriginalValue() {
	return m_originalValue;
}
