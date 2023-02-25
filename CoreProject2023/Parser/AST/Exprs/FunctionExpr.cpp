#include "FunctionExpr.h"
#include <Parser/Visitor/Visitor.h>
#include <Utils/ErrorManager.h>
#include <Module/LLVMGlobals.h>

FunctionExpr::FunctionExpr(Function* func)
    : m_function(func) {
    m_type = m_function->prototype.genType();
    m_safety = m_function->prototype.getQualities().getSafety();
    g_safety.tryUse(m_safety, m_errLine);
}

void FunctionExpr::accept(Visitor* visitor, std::unique_ptr<Expression>& node) {
    visitor->visit(this, node);
}

llvm::Value* FunctionExpr::generate() {
    return m_function->getValue();
}

std::string FunctionExpr::toString() const {
    return m_function->prototype.toString();
}

bool FunctionExpr::isCompileTime() const {
    return true;
}
