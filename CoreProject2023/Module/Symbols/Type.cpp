#include "Type.h"
#include "TypeNode.h"
#include <Module/Module.h>
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
		return -4097;
	}

	if (isConst) {
		return other->isConst ? 0 : -1;
	}

	return other->isConst ? 1 : 0;
}

ArrayType* Type::asArrayType() {
	if (basicType != BasicType::ARRAY) {
		return nullptr;
	}

	return (ArrayType*)this;
}

PointerType* Type::asPointerType() {
	if (basicType < BasicType::DYN_ARRAY || basicType > BasicType::OPTIONAL) {
		return nullptr;
	}

	return (PointerType*)this;
}

TupleType* Type::asTupleType() {
	if (basicType != BasicType::TUPLE) {
		return nullptr;
	}

	return (TupleType*)this;
}

FunctionType* Type::asFunctionType() {
	if (basicType != BasicType::FUNCTION) {
		return nullptr;
	}

	return (FunctionType*)this;
}

TypeNodeType* Type::asTypeNodeType() {
	if (basicType != BasicType::TYPE_NODE) {
		return nullptr;
	}

	return (TypeNodeType*)this;
}

StructType* Type::asStructType() {
	if (basicType != BasicType::STRUCT) {
		return nullptr;
	}

	return (StructType*)this;
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

std::unique_ptr<Type>& Type::getTheVeryType(std::unique_ptr<Type>& type) {
	if (isReference(type->basicType)) {
		return getTheVeryType(type->asPointerType()->elementType);
	}

	return type;
}

const std::unique_ptr<Type>& Type::getTheVeryType(const std::unique_ptr<Type>& type) {
	if (isReference(type->basicType)) {
		return getTheVeryType(type->asPointerType()->elementType);
	}

	return type;
}

ArrayType::ArrayType(std::unique_ptr<Type> elementType, u64 size, bool isConst)
	: elementType(std::move(elementType)), size(size), Type(BasicType::ARRAY, isConst) {

}

std::unique_ptr<Type> ArrayType::copy() const {
	return std::make_unique<ArrayType>(elementType->copy(), size, isConst);
}

bool ArrayType::equals(const std::unique_ptr<Type>& other) const {
	if (!Type::equals(other)) return false;

	const ArrayType* arrType = other->asArrayType();
	if (!elementType->equals(arrType->elementType))
		return false;

	return size == arrType->size;
}

i32 ArrayType::equalsOrLessConstantThan(const std::unique_ptr<Type>& other) const {
	if (other->basicType != BasicType::ARRAY) {
		return -4097;
	}

	const ArrayType* arrType = other->asArrayType();
	if (arrType->size != size) {
		return -4097;
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

llvm::ArrayType* ArrayType::to_llvmArrayType() const {
	return llvm::ArrayType::get(elementType->to_llvm(), size);
}

llvm::Type* ArrayType::to_llvm() const {
	return llvm::PointerType::get(to_llvmArrayType(), 0);
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
	
	if (isReference(this->basicType) && this->elementType->isConst) {
		this->isConst = true;
	}

	if (this->basicType == BasicType::LVAL_REFERENCE && this->isConst) {
		this->basicType = BasicType::XVAL_REFERENCE;
	}
}

std::unique_ptr<Type> PointerType::copy() const {
	return std::make_unique<PointerType>(basicType, elementType->copy(), isConst);
}

bool PointerType::equals(const std::unique_ptr<Type>& other) const {
	if (!Type::equals(other)) {
		return false;
	}

	const PointerType* ptrType = other->asPointerType();
	if (!elementType->equals(ptrType->elementType))
		return false;

	return true;
}

i32 PointerType::equalsOrLessConstantThan(const std::unique_ptr<Type>& other) const {
	if (basicType != other->basicType) {
		return -4097;
	}

	const PointerType* ptrType = other->asPointerType();
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
		case BasicType::LVAL_REFERENCE:
		case BasicType::XVAL_REFERENCE: return llvm::PointerType::get(elementType->to_llvm(), 0);
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
	"[]", "*", "(&)", "&", "&&", "?"
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

	const TupleType* tupType = other->asTupleType();
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
		return -4097;
	}

	const TupleType* tupType = other->asTupleType();
	if (subTypes.size() != tupType->subTypes.size())
		return -4097;

	i32 value = 0;
	for (size_t i = 0; i < subTypes.size(); i++) {
		i32 subVal = subTypes[i]->equalsOrLessConstantThan(tupType->subTypes[i]);
		if (subVal < 0) {
			return subVal;
		}

		value += subVal;
	}

	if (isConst) {
		return value + (other->isConst ? 0 : -1);
	}

	return value + (other->isConst ? 1 : 0);
}

bool TupleType::isEquivalentTo(std::vector<std::unique_ptr<Type>>& types) {
	if (subTypes.size() != types.size()) {
		return false;
	}

	for (size_t i = 0; i < subTypes.size(); i++) {
		if (subTypes[i]->equalsOrLessConstantThan(types[i]) < 0) {
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

	const FunctionType* funcType = other->asFunctionType();
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
		return -4097;
	}

	const FunctionType* funcType = other->asFunctionType();
	if (isVaArgs != funcType->isVaArgs) {
		return -4097;
	}

	if (argTypes.size() != funcType->argTypes.size()) {
		return -4097;
	}

	i32 value = returnType->equalsOrLessConstantThan(funcType->returnType);
	if (value < 0) {
		return value;
	}

	for (size_t i = 0; i < argTypes.size(); i++) {
		i32 subVal = argTypes[i]->equalsOrLessConstantThan(funcType->argTypes[i]);
		if (subVal < 0) {
			return subVal;
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


TypeNodeType::TypeNodeType(std::shared_ptr<TypeNode> node, bool isConst)
	: node(std::move(node)), Type(BasicType::TYPE_NODE, isConst) {

}

std::unique_ptr<Type> TypeNodeType::copy() const {
	return std::make_unique<TypeNodeType>(node, isConst);
}

bool TypeNodeType::equals(const std::unique_ptr<Type>& other) const {
	if (!Type::equals(other)) {
		return false;
	}

	TypeNodeType* typeNT = other->asTypeNodeType();
	if (!node->isEquals(typeNT->node)) {
		return false;
	}

	return node->type->equals(typeNT->node->type);
}

i32 TypeNodeType::equalsOrLessConstantThan(const std::unique_ptr<Type>& other) const {
	if (basicType != other->basicType) {
		return -4097;
	}

	const TypeNodeType* typeNT = other->asTypeNodeType();
	if (!node->isEquals(typeNT->node)) {
		return -4097;
	}

	i32 value = 0;
	if (isConst) {
		return value + (other->isConst ? 0 : -1);
	}

	return value + (other->isConst ? 1 : 0);
}

llvm::Type* TypeNodeType::to_llvm() const {
	return node->llvmType;
}

std::string TypeNodeType::toString() const {
	return (isConst ? "const " : "") + node->name;
}

std::string TypeNodeType::toMangleString() const {
	return node->name + (isConst ? "C" : "");
}

u64 TypeNodeType::getBitSize() const {
	return node->type->getBitSize();
}


StructType::StructType(std::vector<std::unique_ptr<Type>> fieldTypes, bool isConst)
	: fieldTypes(std::move(fieldTypes)), Type(BasicType::STRUCT, isConst) {

}

std::unique_ptr<Type> StructType::copy() const {
	std::vector<std::unique_ptr<Type>> fieldTypesCopy;
	fieldTypesCopy.reserve(fieldTypes.size());
	for (auto& subType : fieldTypes) {
		fieldTypesCopy.push_back(subType->copy());
	}

	return std::make_unique<StructType>(std::move(fieldTypesCopy), isConst);
}

bool StructType::equals(const std::unique_ptr<Type>& other) const {
	if (!Type::equals(other)) {
		return false;
	}

	const StructType* structType = other->asStructType();
	if (fieldTypes.size() != structType->fieldTypes.size()) {
		return false;
	}

	for (size_t i = 0; i < fieldTypes.size(); i++) {
		if (!fieldTypes[i]->equals(structType->fieldTypes[i])) {
			return false;
		}
	}

	return true;
}

i32 StructType::equalsOrLessConstantThan(const std::unique_ptr<Type>& other) const {
	if (basicType != other->basicType) {
		return -4097;
	}

	const StructType* structType = other->asStructType();
	if (fieldTypes.size() != structType->fieldTypes.size())
		return -4097;

	i32 value = 0;
	for (size_t i = 0; i < fieldTypes.size(); i++) {
		i32 subVal = fieldTypes[i]->equalsOrLessConstantThan(structType->fieldTypes[i]);
		if (subVal < 0) {
			return subVal;
		}

		value += subVal;
	}

	if (isConst) {
		return value + (other->isConst ? 0 : -1);
	}

	return value + (other->isConst ? 1 : 0);
}

bool StructType::isEquivalentTo(std::vector<std::unique_ptr<Type>>& types) {
	if (fieldTypes.size() != types.size()) {
		return false;
	}

	for (size_t i = 0; i < fieldTypes.size(); i++) {
		if (fieldTypes[i]->equalsOrLessConstantThan(types[i]) < 0) {
			return false;
		}
	}

	return true;
}

llvm::Type* StructType::to_llvm() const {
	std::vector<llvm::Type*> llvmFieldTypes;
	for (auto& type : fieldTypes) {
		llvmFieldTypes.push_back(type->to_llvm());
	}

	return llvm::StructType::get(g_context, llvmFieldTypes, true);
}

std::string StructType::toString() const {
	std::string result = (isConst ? "const " : "") + std::string("struct {");
	for (auto& type : fieldTypes) {
		result.append("\n\t").append(type->toString()).append(";");
	}

	result += "\n}\n";
	return result;
}

std::string StructType::toMangleString() const {
	std::string result = (isConst ? "structC<" : "struct<");
	for (auto& type : fieldTypes) {
		result.append(type->toMangleString());
	}

	result.back() = '>';
	return result;
}

u64 StructType::getBitSize() const {
	u64 result = 0;
	for (auto& subType : fieldTypes) {
		result += subType->getAlignment() * 8;
	}

	return result;
}



bool isImplicitlyConverible(
	const std::unique_ptr<Type>& from, 
	const std::unique_ptr<Type>& to, 
	bool isFromCompileTime
) {
	if (from->equalsOrLessConstantThan(to) >= 0) {
		return true;
	}

	BasicType bfrom = from->basicType;
	BasicType bto = to->basicType;

	if (bto == BasicType::NO_TYPE) {
		return true;
	}

	if (isTrueReference(bfrom) && isTrueReference(bto) && isReference(from->asPointerType()->elementType->basicType)) {
		return isImplicitlyConverible(from->asPointerType()->elementType, to);
	}

	if (bto == BasicType::XVAL_REFERENCE) {
		const std::unique_ptr<Type>& nextFrom = isReference(bfrom) ?
			from->asPointerType()->elementType
			: from;
		return isImplicitlyConverible(nextFrom, to->asPointerType()->elementType);
	}

	if (bto == BasicType::RVAL_REFERENCE) {
		return isImplicitlyConverible(from, to->asPointerType()->elementType);
	}

	if (bto == BasicType::LVAL_REFERENCE) {
		return isTrueReference(bfrom) && from->asPointerType()->elementType
			->equalsOrLessConstantThan(to->asPointerType()->elementType) >= 0;
	}

	if (isReference(bfrom)) {
		return isImplicitlyConverible(from->asPointerType()->elementType, to);
	}

	if (bfrom == BasicType::TYPE_NODE && from->asTypeNodeType()->node->type->basicType < BasicType::CLASS) {
		return isImplicitlyConverible(from->asTypeNodeType()->node->type, to, isFromCompileTime);
	} else if (bto == BasicType::TYPE_NODE && to->asTypeNodeType()->node->type->basicType < BasicType::CLASS) {
		return isImplicitlyConverible(from, to->asTypeNodeType()->node->type, isFromCompileTime);
	}

	if (bfrom == BasicType::POINTER && isFromCompileTime
		&& (bto == BasicType::POINTER || bto == BasicType::FUNCTION || bto == BasicType::OPTIONAL)) {
		return true;
	}

	if (bfrom == bto && bfrom <= BasicType::STR32) {
		return true;
	}

	if (isString(bfrom) && bto == BasicType::POINTER && isFromCompileTime) {
		if (BasicType charType = to->asPointerType()->elementType->basicType; isChar(charType)) {
			return u8(bfrom) - u8(charType) == u8(BasicType::STR8) - u8(BasicType::C8);
		}
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

	std::vector<std::unique_ptr<Type>> types;
	types.push_back(from->copy());
	if (g_module->chooseConstructor(to, types, { isFromCompileTime }, true)) {
		return true;
	}

	return false;
}

bool isExplicitlyConverible(const std::unique_ptr<Type>& from, const std::unique_ptr<Type>& to) {
	if (isImplicitlyConverible(from, to, true)) { // true because anything convertible in ct is convertible explicitly
		return true;
	}

	BasicType bfrom = from->basicType;
	BasicType bto = to->basicType;

	if (isReference(bfrom)) {
		return isExplicitlyConverible(to, from->asPointerType()->elementType);
	}

	if (!isReference(bto) && from->equalsOrLessConstantThan(to) >= -4096) { // equals not considering constantness
		return true;
	}

	if (bfrom == BasicType::TYPE_NODE) {
		return isExplicitlyConverible(from->asTypeNodeType()->node->type, to);
	} else if (bto == BasicType::TYPE_NODE) {
		return isExplicitlyConverible(from, to->asTypeNodeType()->node->type);
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
		return isNumeric(bto) || isChar(bto) || isTruePointer(bto);
	} else if (isFloat(bfrom)) {
		return isNumeric(bto);
	} else if (isChar(bfrom)) {
		return isChar(bto) || isNumeric(bto);
	} else if (bfrom == BasicType::POINTER || bfrom == BasicType::ARRAY || bfrom == BasicType::FUNCTION) {
		return isInteger(bto) || bto == BasicType::POINTER || bto == BasicType::FUNCTION;
	} else if (bfrom == BasicType::BOOL) {
		return isNumeric(bto) || isChar(bto);
	}

	if (bfrom == BasicType::STRUCT && bto == BasicType::TUPLE) {
		return from->asStructType()->isEquivalentTo(to->asTupleType()->subTypes);
	} else if (bfrom == BasicType::TUPLE && bto == BasicType::STRUCT) {
		return from->asTupleType()->isEquivalentTo(to->asStructType()->fieldTypes);
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

	if (bto == bfrom && bfrom <= BasicType::STR32) {
		return 0;
	}

	if (isReference(bfrom)) {
		if (!isReference(bto)) {
			return evaluateConvertibility(from->asPointerType()->elementType, to);
		} else if (bfrom == bto) {
			return evaluateConvertibility(from->asPointerType()->elementType, to->asPointerType()->elementType);
		}
	}

	if (i32 value = from->equalsOrLessConstantThan(to); value >= 0) {
		return value;
	}

	if (bfrom == BasicType::TYPE_NODE && from->asTypeNodeType()->node->type->basicType < BasicType::CLASS) {
		return evaluateConvertibility(from->asTypeNodeType()->node->type, to, isFromCompileTime);
	} else if (bto == BasicType::TYPE_NODE && to->asTypeNodeType()->node->type->basicType < BasicType::CLASS) {
		return evaluateConvertibility(from, to->asTypeNodeType()->node->type, isFromCompileTime);
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
	BasicType bfirst = first->basicType;
	BasicType bsecond = second->basicType;

	if (isReference(bfirst)) {
		return findCommonType(first->asPointerType()->elementType, second, false, isSecondCompileTime);
	} else if (isReference(bsecond)) {
		return findCommonType(first, second->asPointerType()->elementType, isFirstCompileTime, false);
	}

	if (bfirst == BasicType::TYPE_NODE && first->asTypeNodeType()->node->type->basicType < BasicType::CLASS) {
		return findCommonType(first->asTypeNodeType()->node->type, second, isFirstCompileTime, isSecondCompileTime);
	} else if (bsecond == BasicType::TYPE_NODE && second->asTypeNodeType()->node->type->basicType < BasicType::CLASS) {
		return findCommonType(first, second->asTypeNodeType()->node->type, isFirstCompileTime, isSecondCompileTime);
	}

	if ((isInteger(bfirst) && isInteger(bsecond))
			|| (isFloat(bfirst) && isFloat(bsecond))) {
		if (isFirstCompileTime) {
			return second->copy();
		} else if (isSecondCompileTime) {
			return first->copy();
		}
	}

	if (bfirst == BasicType::POINTER && isInteger(bsecond)) {
		return first->copy();
	} if (bsecond == BasicType::POINTER && isInteger(bfirst)) {
		return second->copy();
	}

	if (isImplicitlyConverible(second, first, isSecondCompileTime)) {
		return first->copy();
	} else if (isImplicitlyConverible(first, second, isFirstCompileTime)) {
		return second->copy();
	}

	return nullptr;
}
