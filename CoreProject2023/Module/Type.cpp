#include "Type.h"
#include "TypeNode.h"
#include <Parser/AST/INode.h>

Type::Type()
	: basicType(BasicType::NO_TYPE), isConst(false) {

}

Type::Type(BasicType basic, bool isConst)
	: basicType(basic), isConst(isConst) {

}

std::unique_ptr<Type> Type::copy() const {
	return std::make_unique<Type>(basicType, isConst);
}

bool Type::equals(const std::unique_ptr<Type>& other) const {
	if (basicType != other->basicType || isConst != other->isConst)
		return false;

	return true;
}

llvm::Type* Type::to_llvm() const {
	return getBasicTypeNode(basicType).llvmType;
}

std::string Type::toString() const {
	return getBasicTypeNode(basicType).name;
}

u64 Type::getBitSize() const {
	return getBasicTypeSize(basicType);
}

u64 Type::getAlignment() const {
	return (this->getBitSize() + 7) / 8;
}

ArrayType::ArrayType(std::unique_ptr<Type> elementType, u64 size, bool isConst)
	: elementType(std::move(elementType)), size(size), Type(BasicType::ARRAY, isConst) {

}

std::unique_ptr<Type> ArrayType::copy() const {
	return std::make_unique<ArrayType>(elementType->copy(), size, isConst);
}

bool ArrayType::equals(const std::unique_ptr<Type>& other) const {
	if (!Type::equals(other)) return false;

	const ArrayType* arrType = (ArrayType*)other.get();
	if (!elementType->equals(arrType->elementType))
		return false;

	return size == arrType->size;
}

llvm::Type* ArrayType::to_llvm() const {
	return llvm::ArrayType::get(elementType->to_llvm(), size);
}

std::string ArrayType::toString() const {
	return elementType->toString() + "[" + std::to_string(size) + "]";
}

u64 ArrayType::getBitSize() const {
	return 64;
}

PointerType::PointerType(BasicType basicType, std::unique_ptr<Type> elementType, bool isConst)
	: elementType(std::move(elementType)), Type(basicType, isConst) {
	ASSERT(basicType >= BasicType::DYN_ARRAY && basicType <= BasicType::OPTIONAL, "wrong basic type");
}

std::unique_ptr<Type> PointerType::copy() const {
	return std::make_unique<PointerType>(basicType, elementType->copy(), isConst);
}

bool PointerType::equals(const std::unique_ptr<Type>& other) const {
	if (!Type::equals(other)) return false;

	const PointerType* ptrType = (PointerType*)other.get();
	if (!elementType->equals(ptrType->elementType))
		return false;

	return true;
}

llvm::Type* PointerType::to_llvm() const {
	switch (basicType) {
	case BasicType::DYN_ARRAY:
		return llvm::StructType::get(g_context,
			{ llvm::PointerType::get(elementType->to_llvm(), 0), llvm::Type::getInt64Ty(g_context) },
			true
		);
	case BasicType::POINTER:
	case BasicType::REFERENCE:
		return llvm::PointerType::get(elementType->to_llvm(), 0);
	case BasicType::RVAL_REFERENCE:
		return elementType->to_llvm();
	case BasicType::OPTIONAL:
		return llvm::StructType::get(g_context,
			{ elementType->to_llvm(), llvm::Type::getInt1Ty(g_context) },
			true
		);
	default: return nullptr;
	}
}

std::string PointerType::toString() const {
	return elementType->toString() + "*";
}

u64 PointerType::getBitSize() const {
	if (basicType == BasicType::RVAL_REFERENCE) {
		return elementType->getBitSize();
	} else if (basicType == BasicType::OPTIONAL) {
		return elementType->getAlignment() * 8 + 1;
	}

	return Type::getBitSize();
}

TupleType::TupleType(std::vector<std::unique_ptr<Type>> subTypes, bool isConst)
	: subTypes(std::move(subTypes)), Type(BasicType::TUPLE, isConst) {
	ASSERT(this->subTypes.size(), "tuple cannot be empty");
}

std::unique_ptr<Type> TupleType::copy() const {
	std::vector<std::unique_ptr<Type>> subTypesCopy;
	subTypesCopy.reserve(subTypes.size());
	for (auto& subType : subTypes) {
		subTypesCopy.push_back(subType->copy());
	}

	return std::make_unique<TupleType>(std::move(subTypesCopy), isConst);
}

bool TupleType::equals(const std::unique_ptr<Type>& other) const {
	if (!Type::equals(other)) return false;

	const TupleType* tupType = (TupleType*)other.get();
	if (subTypes.size() != tupType->subTypes.size())
		return false;

	for (size_t i = 0; i < subTypes.size(); i++) {
		if (!subTypes[i]->equals(tupType->subTypes[i])) {
			return false;
		}
	}

	return true;
}

llvm::Type* TupleType::to_llvm() const {
	std::vector<llvm::Type*> llvmSubTypes;
	for (auto& type : subTypes) {
		llvmSubTypes.push_back(type->to_llvm());
	}

	return llvm::StructType::get(g_context,	llvmSubTypes, true);
}

std::string TupleType::toString() const {
	std::string result = "tuple<";
	for (auto& type : subTypes) {
		result.append(type->toString()).append(", ");
	}

	result.pop_back();
	result.back() = '>';
	return result;
}

u64 TupleType::getBitSize() const {
	u64 result = 0;
	for (auto& subType : subTypes) {
		result += subType->getAlignment() * 8;
	}

	return result;
}

FunctionType::FunctionType(std::unique_ptr<Type> returnType, std::vector<std::unique_ptr<Type>> argTypes, bool isConst)
	: returnType(std::move(returnType)), argTypes(std::move(argTypes)), Type(BasicType::FUNCTION, isConst) {
	
}

std::unique_ptr<Type> FunctionType::copy() const {
	std::vector<std::unique_ptr<Type>> argTypesCopy;
	argTypesCopy.reserve(argTypes.size());
	for (auto& argType : argTypes) {
		argTypesCopy.push_back(std::unique_ptr<Type>(argType->copy()));
	}

	return std::make_unique<FunctionType>(returnType->copy(), std::move(argTypesCopy), isConst);
}

bool FunctionType::equals(const std::unique_ptr<Type>& other) const {
	if (!Type::equals(other)) return false;

	const FunctionType* funcType = (FunctionType*)other.get();
	if (argTypes.size() != funcType->argTypes.size())
		return false;

	if (!returnType->equals(funcType->returnType))
		return false;

	for (size_t i = 0; i < argTypes.size(); i++) {
		if (!argTypes[i]->equals(funcType->argTypes[i])) {
			return false;
		}
	}

	return true;
}

llvm::Type* FunctionType::to_llvm() const {
	std::vector<llvm::Type*> llvmArgTypes;
	for (auto& type : argTypes) {
		llvmArgTypes.push_back(type->to_llvm());
	}

	return llvm::FunctionType::get(returnType->to_llvm(), llvmArgTypes, false);
}

std::string FunctionType::toString() const {
	std::string result = "func " + returnType->toString() + "(";
	for (auto& type : argTypes) {
		result.append(type->toString()).append(", ");
	}

	result.pop_back();
	result.back() = ')';
	return result;
}

u64 FunctionType::getBitSize() const {
	return 64; // pointer
}

bool isImplicitlyConverible(const std::unique_ptr<Type>& from, const std::unique_ptr<Type>& to) {
	if (from->equals(to)) {
		return true;
	}

	BasicType bfrom = from->basicType;
	BasicType bto = to->basicType;
	
	if (isReference(from->basicType)) {
		return isImplicitlyConverible(to, ((PointerType*)from.get())->elementType);
	}

	if (bto == BasicType::BOOL) {
		return isNumeric(bfrom) || isChar(bfrom) || isString(bfrom)
			|| bfrom == BasicType::DYN_ARRAY || bfrom == BasicType::POINTER
			|| bfrom == BasicType::OPTIONAL || bfrom == BasicType::FUNCTION;
	}

	if (isInteger(bfrom)) {
		if (isInteger(bto)) {
			return getBasicTypeSize(bfrom) < getBasicTypeSize(bto);
		} else if (isFloat(bto)) {
			return true;
		}
	} else if (isFloat(bfrom)) {
		if (isFloat(bto)) {
			return getBasicTypeSize(bfrom) < getBasicTypeSize(bto);
		}
	} else if (isChar(bfrom)) {
		if (isChar(bto)) {
			return getBasicTypeSize(bfrom) < getBasicTypeSize(bto);
		}
	} else if (bfrom == BasicType::BOOL) {
		return isInteger(bto);
	}

	return false;
}

bool isExplicitlyConverible(const std::unique_ptr<Type>& from, const std::unique_ptr<Type>& to) {
	if (isImplicitlyConverible(from, to)) {
		return true;
	}

	BasicType bfrom = from->basicType;
	BasicType bto = to->basicType;

	// TODO: delete this string
	if (isString(bfrom) && bto == BasicType::POINTER) return true;

	if (isReference(from->basicType)) {
		return isExplicitlyConverible(to, ((PointerType*)from.get())->elementType);
	}

	if (bto == BasicType::BOOL) {
		return isNumeric(bfrom) || isChar(bfrom) || isString(bfrom)
			|| bfrom == BasicType::DYN_ARRAY || bfrom == BasicType::POINTER
			|| bfrom == BasicType::OPTIONAL || bfrom == BasicType::FUNCTION;
	}

	if (isString(bto)) {
		return isNumeric(bfrom) || isChar(bfrom) || isString(bfrom) || bfrom == BasicType::BOOL
			|| isPointer(bfrom) || bfrom == BasicType::TUPLE || bfrom == BasicType::ENUM;
	}

	if (isInteger(bfrom)) {
		return isNumeric(bto) || isChar(bto);
	} else if (isFloat(bfrom)) {
		return isNumeric(bto);
	} else if (isChar(bfrom)) {
		return isChar(bto) || isNumeric(bto);
	} else if (bfrom == BasicType::POINTER || bfrom == BasicType::ARRAY || bfrom == BasicType::FUNCTION) {
		return isInteger(bto) || bto == BasicType::POINTER || bto == BasicType::FUNCTION;
	} else if (bfrom == BasicType::BOOL) {
		return isNumeric(bto) || isChar(bto);
	}

	return false;
}
