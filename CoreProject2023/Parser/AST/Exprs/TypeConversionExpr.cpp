#include "TypeConversionExpr.h"
#include <Parser/Visitor/Visitor.h>
#include <Utils/ErrorManager.h>
#include <Module/Module.h>
#include <Module/LLVMUtils.h>
#include <Module/LLVMGlobals.h>

TypeConversionExpr::TypeConversionExpr(std::vector<std::unique_ptr<Expression>> args, std::shared_ptr<Type> type)
	: m_args(std::move(args)) {
	m_type = std::move(type);
	if (m_args.size() == 1 && isExplicitlyConverible(m_args[0]->getType(), m_type)) {
		return;
	} else if (isString(m_type->basicType) && m_args.size() == 2) {
		return;
	} else if (m_type->basicType == BasicType::TUPLE && m_args.size() == m_type->asTupleType()->subTypes.size()) {
		return;
	} else {
		m_isConstructor = true;
	}

	m_safety = m_type->safety;
	for (auto& arg : m_args) {
		if (arg->getSafety() == Safety::UNSAFE) {
			m_safety = Safety::UNSAFE;
		}
	}

	g_safety.tryUse(m_safety, m_errLine);
}

void TypeConversionExpr::accept(Visitor* visitor, std::unique_ptr<Expression>& node) {
	visitor->visit(this, node);
}

llvm::Value* TypeConversionExpr::generate() {
	if (m_args.size() == 0 && !isUserDefined(m_type->basicType)) { // e.g. i32()
		return llvm_utils::getDefaultValueOf(m_type);
	} else if (isString(m_type->basicType) && m_args.size() == 2) { // strx(data, size)
		const std::shared_ptr<Type>& dataType = Type::dereference(m_args[0]->getType());
		const std::shared_ptr<Type>& sizeType = Type::dereference(m_args[1]->getType());

		llvm::Value* dataVal = m_args[0]->generate();
		llvm::Value* sizeVal = m_args[1]->generate();

		dataVal = llvm_utils::convertValueTo(dataType, m_args[0]->getType(), dataVal); // removing references
		sizeVal = llvm_utils::convertValueTo(sizeType, m_args[1]->getType(), sizeVal); // removing references

		BasicType charType = BasicType((u8)m_type->basicType - (u8)BasicType::STR8 + (u8)BasicType::C8);
		std::shared_ptr<Type> charPtrType = PointerType::createType(BasicType::POINTER, Type::createType(charType));
		std::shared_ptr<Type> u64Type = Type::createType(BasicType::U64);

		dataVal = llvm_utils::tryImplicitlyConvertTo(charPtrType, dataType, dataVal, m_errLine, m_args[0]->isCompileTime());
		sizeVal = llvm_utils::tryImplicitlyConvertTo(u64Type, sizeType, sizeVal, m_errLine, m_args[1]->isCompileTime());

		return llvm_utils::getStructValue({ dataVal, sizeVal }, m_type);
	} else if (m_type->basicType == BasicType::TUPLE && m_args.size() == m_type->asTupleType()->subTypes.size()) { // tuple<...>(...)
		std::vector<llvm::Value*> values;
		for (size_t i = 0; i < m_args.size(); i++) {
			values.push_back(m_args[i]->generate());
			values.back() = llvm_utils::tryImplicitlyConvertTo(
				m_type->asTupleType()->subTypes[i],
				m_args[i]->getType(),
				values.back(),
				m_errLine,
				m_args[i]->isCompileTime()
			);
		}

		return llvm_utils::getStructValue(values, m_type);
	} else if (m_isConstructor) {
		Function* constructor = chooseConstructor();
		llvm::Function* funcVal = constructor->getValue();

		m_safety = constructor->prototype.getQualities().getSafety();
		g_safety.tryUse(m_safety, m_errLine);

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

std::string TypeConversionExpr::toString() const {
	std::string result = m_type->toString();
	result += '(';

	for (auto& arg : m_args) {
		result += arg->toString();
		result += ", ";
	}

	result.pop_back();
	result.pop_back();

	result += ')';

	return result;
}

Function* TypeConversionExpr::chooseConstructor() {
	std::vector<std::shared_ptr<Type>> argTypes;
	std::vector<bool> isCompileTime;

	for (auto& arg : m_args) {
		argTypes.push_back(arg->getType());
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
