#include "FunctionExpr.h"
#include <Parser/Visitor/Visitor.h>

FunctionExpr::FunctionExpr(Function* func)
    : m_function(func) {
}

void FunctionExpr::accept(Visitor* visitor, std::unique_ptr<Expression>& node) {
    visitor->visit(this, node);
}

llvm::Value* FunctionExpr::generate() {
    return m_function->functionValue;
}
