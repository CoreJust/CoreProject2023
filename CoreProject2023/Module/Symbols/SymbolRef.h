#pragma once
#include "Annotations.h"

enum class SymbolType : u8 {
	NO_SYMBOL = 0,
	VARIABLE,
	FUNCTION,
	CONSTRUCTOR,
	TYPE,
	MODULE
};

// A structure that represents a reference to a symbols in a certain module
struct SymbolRef {
	u64 tokenPos; // index in the list of tokens, used to identify the SymbolRef
	u64 index; // index in corresponding symbol list
	SymbolType symType;
	Visibility visibility;
};