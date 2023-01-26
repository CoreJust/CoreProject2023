#include "ValueExpr.h"
#include <Parser/Visitor/Visitor.h>

ValueExpr::ValueExpr(int val) : m_val(val) { }

void ValueExpr::accept(Visitor* visitor, std::unique_ptr<Expression>& node) {
	visitor->visit(this, node);
}

llvm::Value* ValueExpr::generate() {
	return llvm::ConstantInt::get(g_context, llvm::APInt(32, m_val, true));
}
