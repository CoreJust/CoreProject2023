#pragma once
#include "Variable.h"
#include "Function.h"

namespace llvm_utils {
	llvm::Value* createGlobalVariable(Variable& var, void* initializer); // initializer is Expression*
	llvm::Value* addGlobalVariableFromOtherModule(Variable& var, llvm::Module& module);

	llvm::Value* genFunctionArgumentValue(Function* func, Argument& arg, llvm::Argument* llvmArg);
	llvm::Value* createLocalVariable(llvm::Function* func, const std::unique_ptr<Type>& type, const std::string& name);

	llvm::Constant* getDefaultValueOf(const std::unique_ptr<Type>& type);
	llvm::Constant* getConstantInt(u64 value, u64 bit_width, bool isSigned = false);

	llvm::Value* tryImplicitlyConvertTo(const std::unique_ptr<Type>& to, const std::unique_ptr<Type>& from, 
		llvm::Value* value, u64 errLine, bool isFromCompileTime = false);

	// returns nullptr if conversion is impossible
	llvm::Value* convertValueTo(const std::unique_ptr<Type>& to, const std::unique_ptr<Type>& from, llvm::Value* value);

	llvm::Value* convertToString(const std::unique_ptr<Type>& from, llvm::Value* value, BasicType stringType);
	llvm::Value* convertToBool(const std::unique_ptr<Type>& from, llvm::Value* value);
}