#include "LLVMFunctionManager.h"
#include "FunctionPrototype.h"
#include <Project/Project.h>
#include <Module/Module.h>
#include <Module/LLVMUtils.h>

void LLVMFunctionManager::setInitialValue(llvm::Function* funcValue) {
    m_functionValues[g_module->getPath()] = funcValue;
    m_originalValue = funcValue;
}

llvm::Function* LLVMFunctionManager::getFunctionValueForCurrentModule(FunctionPrototype* func) {
    if (g_settings->compilationMode == CompilationMode::Program) {
        return m_originalValue;
    }

    if (!m_functionValues.contains(g_module->getPath())) {
        m_functionValues[g_module->getPath()] = func->generateImportedFromOtherModule(g_module->getLLVMModule());
    }

    return m_functionValues[g_module->getPath()];
}

llvm::Function* LLVMFunctionManager::getOriginalValue() {
    return m_originalValue;
}
