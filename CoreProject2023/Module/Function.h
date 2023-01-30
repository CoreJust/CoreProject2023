#pragma once
#include "FunctionPrototype.h"
#include "Annotations.h"

struct Function {
	FunctionPrototype prototype;
	FunctionQualities qualities;
	llvm::Function* functionValue;
};