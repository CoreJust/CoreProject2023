#include "AssignmentExpr.h"
#include <Parser/Visitor/Visitor.h>
#include <Module/LLVMUtils.h>
#include <Utils/ErrorManager.h>

AssignmentExpr::AssignmentExpr(std::unique_ptr<Expression> rval, std::unique_ptr<Expression> expr)
	: m_rval(std::move(rval)), m_expr(std::move(expr)) {
	m_type = std::make_unique<PointerType>(BasicType::REFERENCE, m_rval->getType()->copy());
}

void AssignmentExpr::accept(Visitor* visitor, std::unique_ptr<Expression>& node) {
	visitor->visit(this, node);
}

llvm::Value* AssignmentExpr::generate() {
	llvm::Value* rval = m_rval->generateRValue();
	llvm::Value* value = m_expr->generate();

	if (!m_expr->getType()->equals(m_rval->getType())) {
		if (isImplicitlyConverible(m_expr->getType(), m_rval->getType())) {
			llvm_utils::convertValueTo(m_rval->getType(), m_expr->getType(), value);
		} else {
			ErrorManager::typeError(ErrorID::E3101_CANNOT_BE_IMPLICITLY_CONVERTED, m_errLine,
				"value type mismatched in assignment: cannot convert " + m_expr->getType()->toString() + " to " + m_rval->getType()->toString());
		}
	}

	g_builder->CreateStore(value, rval);
	return value;
}

llvm::Value* AssignmentExpr::generateRValue() {
	generate();
	return m_rval->generateRValue();
}
