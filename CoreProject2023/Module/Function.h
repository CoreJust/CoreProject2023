#pragma once
#include "FunctionPrototype.h"

struct Function {
	FunctionPrototype prototype;
	llvm::Function* functionValue;
};