#pragma once
#include <Utils/Defs.h>

enum class BasicType : u8 {
	NO_TYPE = 0, // void

	I8,
	I16,
	I32,
	I64,
	U8,
	U16,
	U32,
	U64,
	F32,
	F64,
	BOOL,
	C8,
	C16,
	C32,

	// string has structure: cx* data, u64 size
	STR8,
	STR16,
	STR32,

	ARRAY, // static array
	DYN_ARRAY, // dynamic array, structure: -type-* data, u64 size
	POINTER,
	REFERENCE, // &, actually a pointer
	RVAL_REFERENCE, // &&, actually the type itself
	OPTIONAL, // structure: -type- data, bool has
	TUPLE,
	FUNCTION,

	CLASS,
	STRUCT,
	ENUM,
	UNION,

	UNKNOWN
};

std::strong_ordering operator<=>(BasicType left, BasicType right);

bool isNumeric(BasicType type);
bool isInteger(BasicType type);
bool isSigned(BasicType type);
bool isUnsigned(BasicType type);
bool isFloat(BasicType type);

bool isChar(BasicType type);
bool isString(BasicType type);

bool isPrimitive(BasicType type);
bool hasSubtypes(BasicType type);
bool isUserDefined(BasicType type);

bool isReference(BasicType type);
bool isPointer(BasicType type); // is pointer-like
bool isTruePointer(BasicType type); // is actually a pointer: static array, pointer, function

// Size of a basic type in bits. If it is varies (e.g. tuple), returns -1
int getBasicTypeSize(BasicType type);

namespace llvm {
	class Type;
};

llvm::Type* basicTypeToLLVM(BasicType type);
