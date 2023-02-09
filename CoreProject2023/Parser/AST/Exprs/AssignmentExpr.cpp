#include "AssignmentExpr.h"
#include <Parser/Visitor/Visitor.h>
#include <Module/LLVMUtils.h>
#include <Utils/ErrorManager.h>

AssignmentExpr::AssignmentExpr(std::unique_ptr<Expression> rval, std::unique_ptr<Expression> expr)
	: m_rval(std::move(rval)), m_expr(std::move(expr)) {
	m_type = std::make_unique<PointerType>(BasicType::REFERENCE, m_rval->getType()->copy());
	m_isRVal = true;

	if (m_rval->getType()->isConst) {
		ErrorManager::typeError(ErrorID::E3057_IS_A_CONSTANT, m_errLine, "tried to assign a value to a constant");
	}
}

void AssignmentExpr::accept(Visitor* visitor, std::unique_ptr<Expression>& node) {
	visitor->visit(this, node);
}

llvm::Value* AssignmentExpr::generate() {
	llvm::Value* rval = m_rval->generateRValue();
	llvm::Value* value = m_expr->generate();
	value = llvm_utils::tryImplicitlyConvertTo(m_rval->getType(), m_expr->getType(), value, m_errLine, m_expr->isCompileTime());

	g_builder->CreateStore(value, rval);
	return value;
}

llvm::Value* AssignmentExpr::generateRValue() {
	generate();
	return m_rval->generateRValue();
}
