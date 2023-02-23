#include "Function.h"

Function::Function(FunctionPrototype proto, std::shared_ptr<LLVMFunctionManager> manager)
    : prototype(std::move(proto)), functionManager(std::move(manager)) {
    if (!functionManager) {
        functionManager = std::make_shared<LLVMFunctionManager>();
    }
}

llvm::Function* Function::getValue() {
    return functionManager->getFunctionValueForCurrentModule(&prototype);
}
