#include "Type.h"
#include "TypeNode.h"
#include <Module/LLVMGlobals.h>

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

i32 Type::equalsOrLessConstantThan(const std::unique_ptr<Type>& other) const {
	if (basicType != other->basicType) {
		return -1;
	}

	if (isConst) {
		return other->isConst ? 0 : -1;
	}

	return other->isConst ? 1 : 0;
}

llvm::Type* Type::to_llvm() const {
	return basicTypeToLLVM(basicType);
}

std::string Type::toString() const {
	return (isConst ? "const " : "") + getBasicTypeNode(basicType).name;
}

std::string Type::toMangleString() const {
	if (basicType > BasicType::STR32) {
		return "";
	}

	static std::string MANGLED_TYPE_NAMES[] = {
		"_v", "_i8", "_i16", "_i32", "_i64", "_u8", "_u16", "_u32", "_u64",
		"_f32", "_f64", "_u1", "_c8", "_c16", "_c32", "_str8", "_str16", "_str32"
	};

	return MANGLED_TYPE_NAMES[(u8)basicType] + (isConst ? "C" : "");
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

i32 ArrayType::equalsOrLessConstantThan(const std::unique_ptr<Type>& other) const {
	if (other->basicType != BasicType::ARRAY) {
		return -1;
	}

	const ArrayType* arrType = (ArrayType*)other.get();
	if (arrType->size != size) {
		return false;
	}

	i32 value = elementType->equalsOrLessConstantThan(arrType->elementType);
	if (value < 0) {
		return value;
	}

	if (isConst) {
		return value + (other->isConst ? 0 : -1);
	}

	return value + (other->isConst ? 1 : 0);
}

llvm::Type* ArrayType::to_llvm() const {
	return llvm::ArrayType::get(elementType->to_llvm(), size);
}

std::string ArrayType::toString() const {
	return elementType->toString() + (isConst ? " const" : "") + "[" + std::to_string(size) + "]";
}

std::string ArrayType::toMangleString() const {
	return elementType->toMangleString() + (isConst ? "C" : "") + "[" + std::to_string(size) + "]";
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
	if (!Type::equals(other)) {
		return false;
	}

	const PointerType* ptrType = (PointerType*)other.get();
	if (!elementType->equals(ptrType->elementType))
		return false;

	return true;
}

i32 PointerType::equalsOrLessConstantThan(const std::unique_ptr<Type>& other) const {
	if (basicType != other->basicType) {
		return -1;
	}

	const PointerType* ptrType = (PointerType*)other.get();
	i32 value = elementType->equalsOrLessConstantThan(ptrType->elementType);
	if (value < 0) {
		return value;
	}

	if (isConst) {
		return value + (other->isConst ? 0 : -1);
	}

	return value + (other->isConst ? 1 : 0);
}

llvm::Type* PointerType::to_llvm() const {
	switch (basicType) {
		case BasicType::DYN_ARRAY:
			return llvm::StructType::get(
				g_context,
				{ // { -type-* data, u64 size }
					llvm::PointerType::get(elementType->to_llvm(), 0), 
					llvm::Type::getInt64Ty(g_context) 
				},
				true // is packed
			);
		case BasicType::POINTER:
		case BasicType::REFERENCE: return llvm::PointerType::get(elementType->to_llvm(), 0);
		case BasicType::RVAL_REFERENCE: return elementType->to_llvm();
		case BasicType::OPTIONAL:
			return llvm::StructType::get(
				g_context,
				{ // { -type- data, bool has }
					elementType->to_llvm(), 
					llvm::Type::getInt1Ty(g_context) 
				},
				true // is packed
			);
	default: return nullptr;
	}
}

std::string POINTER_TYPE_MANGLE_STRING[] = { // starting from 19
	"[]", "*", "&", "&&", "?"
};

std::string PointerType::toString() const {
	return elementType->toString() + (isConst ? " const" : "") + POINTER_TYPE_MANGLE_STRING[(u8)basicType - 19];
}

std::string PointerType::toMangleString() const {
	return elementType->toMangleString() + (isConst ? "C" : "") + POINTER_TYPE_MANGLE_STRING[(u8)basicType - 19];
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
	if (!Type::equals(other)) {
		return false;
	}

	const TupleType* tupType = (TupleType*)other.get();
	if (subTypes.size() != tupType->subTypes.size()) {
		return false;
	}

	for (size_t i = 0; i < subTypes.size(); i++) {
		if (!subTypes[i]->equals(tupType->subTypes[i])) {
			return false;
		}
	}

	return true;
}

i32 TupleType::equalsOrLessConstantThan(const std::unique_ptr<Type>& other) const {
	if (basicType != other->basicType) {
		return -1;
	}

	const TupleType* tupType = (TupleType*)other.get();
	if (subTypes.size() != tupType->subTypes.size())
		return -1;

	i32 value = 0;
	for (size_t i = 0; i < subTypes.size(); i++) {
		i32 subVal = subTypes[i]->equalsOrLessConstantThan(tupType->subTypes[i]);
		if (subVal < 0) {
			return -1;
		}

		value += subVal;
	}

	if (isConst) {
		return value + (other->isConst ? 0 : -1);
	}

	return value + (other->isConst ? 1 : 0);
}

llvm::Type* TupleType::to_llvm() const {
	std::vector<llvm::Type*> llvmSubTypes;
	for (auto& type : subTypes) {
		llvmSubTypes.push_back(type->to_llvm());
	}

	return llvm::StructType::get(g_context,	llvmSubTypes, true);
}

std::string TupleType::toString() const {
	std::string result = (isConst ? "const " : "") + std::string("tuple<");
	for (auto& type : subTypes) {
		result.append(type->toString()).append(", ");
	}

	result.pop_back();
	result.back() = '>';
	return result;
}

std::string TupleType::toMangleString() const {
	std::string result = (isConst ? "tupC<" : "tup<");
	for (auto& type : subTypes) {
		result.append(type->toMangleString());
	}

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

FunctionType::FunctionType(
	std::unique_ptr<Type> returnType, 
	std::vector<std::unique_ptr<Type>> argTypes, 
	bool isVaArgs, 
	bool isConst
) : 
	returnType(std::move(returnType)), 
	argTypes(std::move(argTypes)), 
	isVaArgs(isVaArgs), 
	Type(BasicType::FUNCTION, isConst) {
	
}

std::unique_ptr<Type> FunctionType::copy() const {
	std::vector<std::unique_ptr<Type>> argTypesCopy;
	argTypesCopy.reserve(argTypes.size());
	for (auto& argType : argTypes) {
		argTypesCopy.push_back(std::unique_ptr<Type>(argType->copy()));
	}

	return std::make_unique<FunctionType>(returnType->copy(), std::move(argTypesCopy), isVaArgs, isConst);
}

bool FunctionType::equals(const std::unique_ptr<Type>& other) const {
	if (!Type::equals(other)) return false;

	const FunctionType* funcType = (FunctionType*)other.get();
	if (isVaArgs != funcType->isVaArgs) {
		return false;
	}

	if (argTypes.size() != funcType->argTypes.size()) {
		return false;
	}

	if (!returnType->equals(funcType->returnType)) {
		return false;
	}

	for (size_t i = 0; i < argTypes.size(); i++) {
		if (!argTypes[i]->equals(funcType->argTypes[i])) {
			return false;
		}
	}

	return true;
}

i32 FunctionType::equalsOrLessConstantThan(const std::unique_ptr<Type>& other) const {
	if (basicType != other->basicType) {
		return -1;
	}

	const FunctionType* funcType = (FunctionType*)other.get();
	if (isVaArgs != funcType->isVaArgs) {
		return -1;
	}

	if (argTypes.size() != funcType->argTypes.size()) {
		return -1;
	}

	i32 value = returnType->equalsOrLessConstantThan(funcType->returnType);
	if (value < 0) {
		return -1;
	}

	for (size_t i = 0; i < argTypes.size(); i++) {
		i32 subVal = argTypes[i]->equalsOrLessConstantThan(funcType->argTypes[i]);
		if (subVal < 0) {
			return -1;
		}

		value += subVal;
	}

	if (isConst) {
		return value + (other->isConst ? 0 : -1);
	}

	return value + (other->isConst ? 1 : 0);
}

llvm::FunctionType* FunctionType::to_llvmFunctionType() const {
	std::vector<llvm::Type*> llvmArgTypes;
	for (auto& type : argTypes) {
		llvmArgTypes.push_back(type->to_llvm());
	}

	return llvm::FunctionType::get(returnType->to_llvm(), llvmArgTypes, isVaArgs);
}

llvm::Type* FunctionType::to_llvm() const {
	return llvm::PointerType::get(to_llvmFunctionType(), 0);
}

std::string FunctionType::toString() const {
	std::string result = std::string(isConst ? "const " : "") + "func " + returnType->toString() + "(";
	for (auto& type : argTypes) {
		result.append(type->toString()).append(", ");
	}

	result.pop_back();
	result.pop_back();
	if (isVaArgs) {
		result += ", ...";
	}

	result += ')';
	return result;
}

std::string FunctionType::toMangleString() const {
	std::string result = (isConst ? "funcC" : "func") + returnType->toMangleString() + "(";
	for (auto& type : argTypes) {
		result.append(type->toMangleString());
	}

	if (isVaArgs) {
		result += "_...";
	}

	result += ')';
	return result;
}

u64 FunctionType::getBitSize() const {
	return 64; // pointer
}

bool isImplicitlyConverible(
	const std::unique_ptr<Type>& from, 
	const std::unique_ptr<Type>& to, 
	bool isFromCompileTime
) {
	if (to->basicType == BasicType::NO_TYPE) {
		return true;
	}

	if (from->equalsOrLessConstantThan(to) >= 0) {
		return true;
	}

	BasicType bfrom = from->basicType;
	BasicType bto = to->basicType;

	if (bfrom == bto && bfrom <= BasicType::STR32) {
		return true;
	}

	if (bfrom == BasicType::POINTER && isFromCompileTime
		&& (bto == BasicType::POINTER || bto == BasicType::FUNCTION || bto == BasicType::OPTIONAL)) {
		return true;
	}
	
	if (isReference(from->basicType)) {
		return isImplicitlyConverible(((PointerType*)from.get())->elementType, to);
	} else if (isReference(to->basicType)) {
		return isImplicitlyConverible(from, ((PointerType*)to.get())->elementType, isFromCompileTime);
	}

	if (bto == BasicType::BOOL) {
		return isNumeric(bfrom) || isChar(bfrom) || isString(bfrom)
			|| bfrom == BasicType::DYN_ARRAY || bfrom == BasicType::POINTER
			|| bfrom == BasicType::OPTIONAL || bfrom == BasicType::FUNCTION;
	}

	if (isInteger(bfrom)) {
		if (isInteger(bto)) {
			return getBasicTypeSize(bfrom) < getBasicTypeSize(bto) || isFromCompileTime;
		} else if (isFloat(bto)) {
			return true;
		}
	} else if (isFloat(bfrom)) {
		if (isFloat(bto)) {
			return getBasicTypeSize(bfrom) < getBasicTypeSize(bto) || isFromCompileTime;
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

i32 evaluateConvertibility(
	const std::unique_ptr<Type>& from,
	const std::unique_ptr<Type>& to,
	bool isFromCompileTime
) {
	BasicType bfrom = from->basicType;
	BasicType bto = to->basicType;

	if (isReference(bfrom) && !isReference(bto)) {
		return evaluateConvertibility(((PointerType*)from.get())->elementType, to);
	}

	if (i32 value = from->equalsOrLessConstantThan(to); value >= 0) {
		return value;
	}

	// Prefered implicit conversions
	if ((isInteger(bfrom) && isInteger(bto))
		|| (isChar(bfrom) && isChar(bto))
		|| (isFloat(bfrom) && isFloat(bto))) {
		if (from->getBitSize() < to->getBitSize()) {
			return 1024;
		}
	}

	if (isString(bfrom) && isString(bto)) {
		if (bfrom < bto) {
			return 1024;
		}
	}

	if (isImplicitlyConverible(from, to, isFromCompileTime)) {
		return 2048;
	}

	return -1;
}

std::unique_ptr<Type> findCommonType(
	const std::unique_ptr<Type>& first, 
	const std::unique_ptr<Type>& second,
	bool isFirstCompileTime, 
	bool isSecondCompileTime
) {
	if ((isInteger(first->basicType) && isInteger(second->basicType))
			|| (isFloat(first->basicType) && isFloat(second->basicType))) {
		if (isFirstCompileTime) {
			return second->copy();
		} else if (isSecondCompileTime) {
			return first->copy();
		}
	}

	if (first->basicType == BasicType::POINTER && isInteger(second->basicType)) {
		return first->copy();
	} if (second->basicType == BasicType::POINTER && isInteger(first->basicType)) {
		return second->copy();
	}
	if (isImplicitlyConverible(second, first, isSecondCompileTime)) {
		return first->copy();
	} else if (isImplicitlyConverible(first, second, isFirstCompileTime)) {
		return second->copy();
	}

	return nullptr;
}
