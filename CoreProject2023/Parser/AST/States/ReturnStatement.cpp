#include "ReturnStatement.h"
#include "../Decls/FunctionDeclaration.h"
#include <Parser/Visitor/Visitor.h>
#include <Module/LLVMUtils.h>
#include <Utils/ErrorManager.h>
#include <Module/LLVMGlobals.h>

ReturnStatement::ReturnStatement(std::unique_ptr<Expression> expr) 
	: m_expr(std::move(expr)) {

}

void ReturnStatement::accept(Visitor* visitor, std::unique_ptr<Statement>& node) {
	visitor->visit(this, node);
}

void ReturnStatement::generate() {
	if (m_expr) {
		llvm::Value* value = m_expr->generate();
		value = llvm_utils::tryImplicitlyConvertTo(
			g_function->prototype.getReturnType(),
			m_expr->getType(),
			value,
			m_errLine,
			m_expr->isCompileTime()
		);

		if (m_expr->getType()->basicType == BasicType::NO_TYPE) {
			g_builder->CreateRetVoid();
		} else {
			g_builder->CreateRet(value);
		}
	} else {
		if (g_function->prototype.getReturnType()->basicType != BasicType::NO_TYPE) {
			ErrorManager::typeError(
				ErrorID::E3101_CANNOT_BE_IMPLICITLY_CONVERTED, 
				m_errLine,
				"no_type to " + g_function->prototype.getReturnType()->toString()
			);
		}

		g_builder->CreateRetVoid();
	}

	throw new TerminatorAdded;
}
