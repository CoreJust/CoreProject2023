#include "AsExpr.h"
#include <Parser/Visitor/Visitor.h>
#include <Module/LLVMGlobals.h>
#include <Module/LLVMUtils.h>

AsExpr::AsExpr(std::unique_ptr<Expression> arg, std::shared_ptr<Type> type)
	: m_arg(std::move(arg)) {
	m_type = std::move(type);
	m_safety = Safety::UNSAFE;
	g_safety.tryUse(m_safety, m_errLine);
}

void AsExpr::accept(Visitor* visitor, std::unique_ptr<Expression>& node) {
	visitor->visit(this, node);
}

llvm::Value* AsExpr::generate() {
	llvm::Value* val = m_arg->generate();
	if (m_arg->isLVal()) {
		val = llvm_utils::convertValueTo(
			Type::dereference(m_arg->getType()), 
			m_arg->getType()->asPointerType()->elementType,
			val
		);

		if (isTrueReference(m_type->basicType)) {
			return g_builder->CreateBitCast(val, m_type->to_llvm());
		} else {
			return g_builder->CreateLoad(m_type->to_llvm(), val);
		}
	} else { // The converted value is not an lvalue
		if (val->getType()->isPointerTy()) {
			val = g_builder->CreatePtrToInt(val, llvm::Type::getInt64Ty(g_context));
		}

		llvm::Type* llvmType = m_type->to_llvm();
		if (llvmType->isPointerTy()) {
			val = g_builder->CreateBitCast(val, llvm::Type::getInt64Ty(g_context));
			return g_builder->CreateIntToPtr(val, llvmType);
		}

		if (llvmType->isSingleValueType() && m_type->getBitSize() == m_arg->getType()->getBitSize()) {
			return g_builder->CreateBitCast(val, llvmType);
		}

		llvm::Value* alloc = nullptr;
		if (m_type->getBitSize() >= m_arg->getType()->getBitSize()) {
			alloc = g_builder->CreateAlloca(llvmType, 0, "$as_cast");
		} else {
			alloc = g_builder->CreateAlloca(m_arg->getType()->to_llvm(), 0, "$as_cast");
		}

		g_builder->CreateStore(val, alloc);
		return g_builder->CreateLoad(llvmType, alloc);
	}
}

std::string AsExpr::toString() const {
	return "(" + m_arg->toString() + " as " + m_type->toString() + ")";
}
