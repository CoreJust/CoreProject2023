#include "TypeConversionExpr.h"
#include <Parser/Visitor/Visitor.h>
#include <Utils/ErrorManager.h>
#include <Module/Module.h>
#include <Module/LLVMUtils.h>
#include <Module/LLVMGlobals.h>

TypeConversionExpr::TypeConversionExpr(std::vector<std::unique_ptr<Expression>> args, std::unique_ptr<Type> type)
	: m_args(std::move(args)) {
	m_type = std::move(type);
	if (m_args.size() == 1 && isExplicitlyConverible(m_args[0]->getType(), m_type)) {
		return;
	} else {
		m_isConstructor = true;
	}
}

void TypeConversionExpr::accept(Visitor* visitor, std::unique_ptr<Expression>& node) {
	visitor->visit(this, node);
}

llvm::Value* TypeConversionExpr::generate() {
	if (m_args.size() == 0 && !isUserDefined(m_type->basicType)) { // e.g. i32()
		return llvm_utils::getDefaultValueOf(m_type);
	} else if (m_isConstructor) {
		Function* constructor = chooseConstructor();
		llvm::Function* funcVal = constructor->functionValue;

		std::vector<llvm::Value*> argValues;
		for (size_t i = 0; i < m_args.size(); i++) {
			argValues.push_back(m_args[i]->generate());

			if (i < constructor->prototype.args().size()) { // not va_args
				argValues.back() = llvm_utils::tryImplicitlyConvertTo(
					constructor->prototype.args()[i].type, // to type
					m_args[i]->getType(), // from type
					argValues.back(), // llvm value to be converted
					m_errLine, // the line the expression is at
					m_args[i]->isCompileTime() // is the expression available in compile time
				);
			}
		}
		
		return g_builder->CreateCall(
			(llvm::FunctionType*)constructor->prototype.genType()->to_llvmFunctionType(),
			funcVal,
			argValues
		);
	} else {
		llvm::Value* value = m_args[0]->generate();
		if (m_type->equals(m_args[0]->getType())) {
			return value;
		}

		return llvm_utils::convertValueTo(m_type, m_args[0]->getType(), value);
	}
}

Function* TypeConversionExpr::chooseConstructor() {
	std::vector<std::unique_ptr<Type>> argTypes;
	std::vector<bool> isCompileTime;

	for (auto& arg : m_args) {
		argTypes.push_back(arg->getType()->copy());
		isCompileTime.push_back(arg->isCompileTime());
	}

	Function* func = g_module->chooseConstructor(m_type, argTypes, isCompileTime, false);
	if (!func) {
		std::string error = "constructor for arguments ";

		error += m_type->toString();
		error += '(';
		for (auto& type : argTypes) {
			error += type->toString();
			error += ", ";
		}

		if (argTypes.size()) {
			error.pop_back();
			error.pop_back();
		}

		error += ')';
		error += " not found";

		ErrorManager::typeError(
			ErrorID::E3104_NO_SUITABLE_CONSTRUCTOR,
			m_errLine,
			error
		);
	}

	return func;
}
