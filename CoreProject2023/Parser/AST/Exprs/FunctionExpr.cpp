#include "FunctionExpr.h"
#include <Parser/Visitor/Visitor.h>
#include <Utils/ErrorManager.h>
#include <Module/LLVMGlobals.h>

FunctionExpr::FunctionExpr(Function* func)
    : m_function(func) {
    m_type = func->prototype.genType();
}

void FunctionExpr::accept(Visitor* visitor, std::unique_ptr<Expression>& node) {
    visitor->visit(this, node);
}

llvm::Value* FunctionExpr::generate() {
    return m_function->getValue();
}

bool FunctionExpr::isCompileTime() const {
    return true;
}
