#pragma once
#include <string>
#include <vector>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include "Annotations.h"
#include "BasicType.h"

struct TypeNode;

class ArrayType;
class PointerType;
class TupleType;
class FunctionType;
class TypeNodeType;
class StructType;

// Any type is created in a single instance and never changed

class Type {
protected:
	static std::vector<std::vector<std::shared_ptr<Type>>> s_typeInstances[2]; // [isConst][basicType][hash]

protected:
	u64 m_hash = 0; // unique to each Type with a certain constness and basic type, the index of the type in instances

public:
	const BasicType basicType;
	const bool isConst;
	Safety safety = Safety::SAFE;

public:
	Type();
	Type(BasicType basic, bool isConst = false);

	// -1 = not change, 0 = set to false, 1 = set to true
	virtual std::shared_ptr<Type> copy(i32 makeConst = -1) const;

	bool equals(const std::shared_ptr<Type>& other) const;

	// < 0 if not equal, < -4096 if not equal at all (not equal not considering constantness)
	virtual i32 equalsOrLessConstantThan(const std::shared_ptr<Type>& other) const;

	// returns this as a corresponding type is it is such (according to the basicType) and nullptr otherwise
	ArrayType* asArrayType();
	PointerType* asPointerType();
	TupleType* asTupleType();
	FunctionType* asFunctionType();
	TypeNodeType* asTypeNodeType();
	StructType* asStructType();

	virtual llvm::Type* to_llvm() const;
	virtual std::string toString() const;
	virtual std::string toMangleString() const;

	virtual u64 getBitSize() const;
	u64 getAlignment() const;
	u64 getHash() const;

public:
	// returns contained type for references, or returns the argument
	static std::shared_ptr<Type>& dereference(std::shared_ptr<Type>& type);
	static const std::shared_ptr<Type>& dereference(const std::shared_ptr<Type>& type);

public:
	// To be used instead of constructor
	static std::shared_ptr<Type> createType(BasicType type, bool isConst = false);
};


class ArrayType final : public Type {
public:
	std::shared_ptr<Type> elementType;
	u64 size;

public:
	ArrayType(
		std::shared_ptr<Type> elementType, 
		u64 size, 
		bool isConst = false
	);

	// -1 = not change, 0 = set to false, 1 = set to true
	virtual std::shared_ptr<Type> copy(i32 makeConst = -1) const override;

	i32 equalsOrLessConstantThan(const std::shared_ptr<Type>& other) const override; // < 0 if not equal, < -4096 if not equal at all

	llvm::ArrayType* to_llvmArrayType() const;
	llvm::Type* to_llvm() const override;
	std::string toString() const override;
	std::string toMangleString() const override;
	
	u64 getBitSize() const override;

public:
	static std::shared_ptr<ArrayType> createType(
		std::shared_ptr<Type> elementType,
		u64 size,
		bool isConst = false
	);
};


// Dynamic array, pointer, references, optional
class PointerType final : public Type {
public:
	std::shared_ptr<Type> elementType;

public:
	PointerType(
		BasicType basicType, 
		std::shared_ptr<Type> elementType, 
		bool isConst = false
	);

	// -1 = not change, 0 = set to false, 1 = set to true
	virtual std::shared_ptr<Type> copy(i32 makeConst = -1) const override;

	i32 equalsOrLessConstantThan(const std::shared_ptr<Type>& other) const override; // < 0 if not equal, < -4096 if not equal at all

	llvm::Type* to_llvm() const override;
	std::string toString() const override;
	std::string toMangleString() const override;

	u64 getBitSize() const override;

public:
	static std::shared_ptr<PointerType> createType(
		BasicType basicType,
		std::shared_ptr<Type> elementType,
		bool isConst = false
	);
};


class TupleType final : public Type {
public:
	std::vector<std::shared_ptr<Type>> subTypes;

public:
	TupleType(
		std::vector<std::shared_ptr<Type>> subTypes, 
		bool isConst = false
	);

	// -1 = not change, 0 = set to false, 1 = set to true
	virtual std::shared_ptr<Type> copy(i32 makeConst = -1) const override;

	i32 equalsOrLessConstantThan(const std::shared_ptr<Type>& other) const override; // < 0 if not equal, < -4096 if not equal at all

	bool isEquivalentTo(std::vector<std::shared_ptr<Type>>& types);

	llvm::Type* to_llvm() const override;
	std::string toString() const override;
	std::string toMangleString() const override;

	u64 getBitSize() const override;

public:
	static std::shared_ptr<TupleType> createType(
		std::vector<std::shared_ptr<Type>> subTypes,
		bool isConst = false
	);
};


class FunctionType final : public Type {
public:
	std::shared_ptr<Type> returnType;
	std::vector<std::shared_ptr<Type>> argTypes;
	bool isVaArgs;

public:
	FunctionType(
		std::shared_ptr<Type> returnType, 
		std::vector<std::shared_ptr<Type>> argTypes, 
		bool isVaArgs, 
		bool isConst = false
	);

	// -1 = not change, 0 = set to false, 1 = set to true
	virtual std::shared_ptr<Type> copy(i32 makeConst = -1) const override;

	i32 equalsOrLessConstantThan(const std::shared_ptr<Type>& other) const override; // < 0 if not equal, < -4096 if not equal at all

	llvm::FunctionType* to_llvmFunctionType() const;
	llvm::Type* to_llvm() const override;
	std::string toString() const override;
	std::string toMangleString() const override;

	u64 getBitSize() const override;

public:
	static std::shared_ptr<FunctionType> createType(
		std::shared_ptr<Type> returnType,
		std::vector<std::shared_ptr<Type>> argTypes,
		bool isVaArgs,
		bool isConst = false
	);
};

class TypeNodeType final : public Type {
public:
	std::shared_ptr<TypeNode> node;

public:
	TypeNodeType(
		std::shared_ptr<TypeNode> node,
		bool isConst = false
	);

	// -1 = not change, 0 = set to false, 1 = set to true
	virtual std::shared_ptr<Type> copy(i32 makeConst = -1) const override;

	i32 equalsOrLessConstantThan(const std::shared_ptr<Type>& other) const override; // < 0 if not equal, < -4096 if not equal at all

	llvm::Type* to_llvm() const override;
	std::string toString() const override;
	std::string toMangleString() const override;

	u64 getBitSize() const override;

public:
	static std::shared_ptr<TypeNodeType> createType(
		std::shared_ptr<TypeNode> node,
		bool isConst = false
	);
};

// Note: to be used in TypeNode, otherwise must be wrapped in TypeNodeType
class StructType final : public Type {
public:
	std::vector<std::shared_ptr<Type>> fieldTypes;

public:
	StructType(
		std::vector<std::shared_ptr<Type>> fieldTypes,
		bool isConst = false
	);

	// -1 = not change, 0 = set to false, 1 = set to true
	virtual std::shared_ptr<Type> copy(i32 makeConst = -1) const override;

	i32 equalsOrLessConstantThan(const std::shared_ptr<Type>& other) const override; // < 0 if not equal, < -4096 if not equal at all

	bool isEquivalentTo(std::vector<std::shared_ptr<Type>>& types);

	llvm::Type* to_llvm() const override;
	std::string toString() const override;
	std::string toMangleString() const override;

	u64 getBitSize() const override;

public:
	static std::shared_ptr<StructType> createType(
		std::vector<std::shared_ptr<Type>> fieldTypes,
		bool isConst = false
	);
};


bool isImplicitlyConverible(
	const std::shared_ptr<Type>& from, 
	const std::shared_ptr<Type>& to, 
	bool isFromCompileTime = false
);

bool isExplicitlyConverible(
	const std::shared_ptr<Type>& from, 
	const std::shared_ptr<Type>& to
);

// Evaluates the value of convertibility to the other type
// 0 - equal
// <256 - equal not considering constantness
// otherwise - unequal
// Negative value - cannot be implicitly converted at all
i32 evaluateConvertibility(
	const std::shared_ptr<Type>& from,
	const std::shared_ptr<Type>& to,
	bool isFromCompileTime = false);

// Returns the type both types can be converted to, returns nullptr if cannot be converted
std::shared_ptr<Type> findCommonType(
	const std::shared_ptr<Type>& first, 
	const std::shared_ptr<Type>& second,
	bool isFirstCompileTime = false, 
	bool isSecondCompileTime = false
);
