#pragma once
#include <string>
#include <vector>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include "Annotations.h"
#include "BasicType.h"

struct TypeNode;

class Type {
public:
	BasicType basicType;
	bool isConst;

public:
	Type();
	Type(BasicType basic, bool isConst = false);

	virtual std::unique_ptr<Type> copy() const;

	virtual bool equals(const std::unique_ptr<Type>& other) const;

	// < 0 if not equal, < -4096 if not equal at all
	virtual i32 equalsOrLessConstantThan(const std::unique_ptr<Type>& other) const;

	virtual llvm::Type* to_llvm() const;
	virtual std::string toString() const;
	virtual std::string toMangleString() const;

	virtual u64 getBitSize() const;
	u64 getAlignment() const;
};


class ArrayType final : public Type {
public:
	std::unique_ptr<Type> elementType;
	u64 size;

public:
	ArrayType(
		std::unique_ptr<Type> elementType, 
		u64 size, 
		bool isConst = false
	);

	std::unique_ptr<Type> copy() const override;

	bool equals(const std::unique_ptr<Type>& other) const override;
	i32 equalsOrLessConstantThan(const std::unique_ptr<Type>& other) const override; // < 0 if not equal, < -4096 if not equal at all

	llvm::Type* to_llvm() const override;
	std::string toString() const override;
	std::string toMangleString() const override;
	
	u64 getBitSize() const override;
};


// Dynamic array, pointer, references, optional
class PointerType final : public Type {
public:
	std::unique_ptr<Type> elementType;

public:
	PointerType(
		BasicType basicType, 
		std::unique_ptr<Type> elementType, 
		bool isConst = false
	);

	std::unique_ptr<Type> copy() const override;

	bool equals(const std::unique_ptr<Type>& other) const override;
	i32 equalsOrLessConstantThan(const std::unique_ptr<Type>& other) const override; // < 0 if not equal, < -4096 if not equal at all

	llvm::Type* to_llvm() const override;
	std::string toString() const override;
	std::string toMangleString() const override;

	u64 getBitSize() const override;
};


class TupleType final : public Type {
public:
	std::vector<std::unique_ptr<Type>> subTypes;

public:
	TupleType(
		std::vector<std::unique_ptr<Type>> subTypes, 
		bool isConst = false
	);

	std::unique_ptr<Type> copy() const override;

	bool equals(const std::unique_ptr<Type>& other) const override;
	i32 equalsOrLessConstantThan(const std::unique_ptr<Type>& other) const override; // < 0 if not equal, < -4096 if not equal at all

	bool isEquivalentTo(std::vector<std::unique_ptr<Type>>& types);

	llvm::Type* to_llvm() const override;
	std::string toString() const override;
	std::string toMangleString() const override;

	u64 getBitSize() const override;
};


class FunctionType final : public Type {
public:
	std::unique_ptr<Type> returnType;
	std::vector<std::unique_ptr<Type>> argTypes;
	bool isVaArgs;

public:
	FunctionType(
		std::unique_ptr<Type> returnType, 
		std::vector<std::unique_ptr<Type>> argTypes, 
		bool isVaArgs, 
		bool isConst = false
	);

	std::unique_ptr<Type> copy() const override;

	bool equals(const std::unique_ptr<Type>& other) const override;
	i32 equalsOrLessConstantThan(const std::unique_ptr<Type>& other) const override; // < 0 if not equal, < -4096 if not equal at all

	llvm::FunctionType* to_llvmFunctionType() const;
	llvm::Type* to_llvm() const override;
	std::string toString() const override;
	std::string toMangleString() const override;

	u64 getBitSize() const override;
};

class TypeNodeType final : public Type {
public:
	std::shared_ptr<TypeNode> node;

public:
	TypeNodeType(
		std::shared_ptr<TypeNode> node,
		bool isConst = false
	);

	std::unique_ptr<Type> copy() const override;

	bool equals(const std::unique_ptr<Type>& other) const override;
	i32 equalsOrLessConstantThan(const std::unique_ptr<Type>& other) const override; // < 0 if not equal, < -4096 if not equal at all

	llvm::Type* to_llvm() const override;
	std::string toString() const override;
	std::string toMangleString() const override;

	u64 getBitSize() const override;
};

// Note: to be used in TypeNode, otherwise must be wrapped in TypeNodeType
class StructType final : public Type {
public:
	std::vector<std::unique_ptr<Type>> fieldTypes;

public:
	StructType(
		std::vector<std::unique_ptr<Type>> fieldTypes,
		bool isConst = false
	);

	std::unique_ptr<Type> copy() const override;

	bool equals(const std::unique_ptr<Type>& other) const override;
	i32 equalsOrLessConstantThan(const std::unique_ptr<Type>& other) const override; // < 0 if not equal, < -4096 if not equal at all

	bool isEquivalentTo(std::vector<std::unique_ptr<Type>>& types);

	llvm::Type* to_llvm() const override;
	std::string toString() const override;
	std::string toMangleString() const override;

	u64 getBitSize() const override;
};


// TODO: add consideration of user-defined types
bool isImplicitlyConverible(
	const std::unique_ptr<Type>& from, 
	const std::unique_ptr<Type>& to, 
	bool isFromCompileTime = false
);

bool isExplicitlyConverible(
	const std::unique_ptr<Type>& from, 
	const std::unique_ptr<Type>& to
);

// Evaluates the value of convertibility to the other type
// 0 - equal
// <256 - equal not considering constantness
// otherwise - unequal
// Negative value - cannot be implicitly converted at all
i32 evaluateConvertibility(
	const std::unique_ptr<Type>& from,
	const std::unique_ptr<Type>& to,
	bool isFromCompileTime = false);

// Returns the type both types can be converted to, returns nullptr if cannot be converted
std::unique_ptr<Type> findCommonType(
	const std::unique_ptr<Type>& first, 
	const std::unique_ptr<Type>& second,
	bool isFirstCompileTime = false, 
	bool isSecondCompileTime = false
);
