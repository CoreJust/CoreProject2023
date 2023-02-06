#pragma once
#include <string>
#include <llvm/IR/Type.h>
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

	virtual llvm::Type* to_llvm() const;
	virtual std::string toString() const;

	virtual u64 getBitSize() const;
	u64 getAlignment() const;
};

class ArrayType : public Type {
public:
	std::unique_ptr<Type> elementType;
	u64 size;

public:
	ArrayType(std::unique_ptr<Type> elementType, u64 size, bool isConst = false);

	std::unique_ptr<Type> copy() const override;

	bool equals(const std::unique_ptr<Type>& other) const override;

	llvm::Type* to_llvm() const override;
	std::string toString() const override;
	
	u64 getBitSize() const override;
};

// Dynamic array, pointer, references, optional
class PointerType : public Type {
public:
	std::unique_ptr<Type> elementType;

public:
	PointerType(BasicType basicType, std::unique_ptr<Type> elementType, bool isConst = false);

	std::unique_ptr<Type> copy() const override;

	bool equals(const std::unique_ptr<Type>& other) const override;

	llvm::Type* to_llvm() const override;
	std::string toString() const override;

	u64 getBitSize() const override;
};

class TupleType : public Type {
public:
	std::vector<std::unique_ptr<Type>> subTypes;

public:
	TupleType(std::vector<std::unique_ptr<Type>> subTypes, bool isConst = false);

	std::unique_ptr<Type> copy() const override;

	bool equals(const std::unique_ptr<Type>& other) const override;

	llvm::Type* to_llvm() const override;
	std::string toString() const override;

	u64 getBitSize() const override;
};

class FunctionType : public Type {
public:
	std::unique_ptr<Type> returnType;
	std::vector<std::unique_ptr<Type>> argTypes;

public:
	FunctionType(std::unique_ptr<Type> returnType, std::vector<std::unique_ptr<Type>> argTypes, bool isConst = false);

	std::unique_ptr<Type> copy() const override;

	bool equals(const std::unique_ptr<Type>& other) const override;

	llvm::Type* to_llvm() const override;
	std::string toString() const override;

	u64 getBitSize() const override;
};

// TODO: user-defined types

// TODO: add consideration of user-defined types
bool isImplicitlyConverible(const std::unique_ptr<Type>& from, const std::unique_ptr<Type>& to);
bool isExplicitlyConverible(const std::unique_ptr<Type>& from, const std::unique_ptr<Type>& to);
