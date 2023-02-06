#include "TypeConversionExpr.h"
#include <Parser/Visitor/Visitor.h>
#include <Utils/ErrorManager.h>
#include <Module/LLVMUtils.h>

TypeConversionExpr::TypeConversionExpr(std::unique_ptr<Expression> expr, std::unique_ptr<Type> type)
	: m_expr(std::move(expr)) {
	m_type = std::move(type);
	if (!isExplicitlyConverible(m_expr->getType(), m_type)) {
		ErrorManager::typeError(ErrorID::E3102_CANNOT_BE_EXPLICITLY_CONVERTED, m_errLine,
			m_expr->getType()->toString() + " cannot be converted to " + m_type->toString());
	}
}

void TypeConversionExpr::accept(Visitor* visitor, std::unique_ptr<Expression>& node) {
	visitor->visit(this, node);
}

llvm::Value* TypeConversionExpr::generate() {
	llvm::Value* value = m_expr->generate();
	if (m_type->equals(m_expr->getType())) {
		return value;
	}

	return llvm_utils::convertValueTo(m_type, m_expr->getType(), value);
}
