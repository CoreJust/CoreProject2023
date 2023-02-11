#pragma once
#include "FunctionPrototype.h"
#include "Annotations.h"

struct Function {
	FunctionPrototype prototype;
	llvm::Function* functionValue;
};