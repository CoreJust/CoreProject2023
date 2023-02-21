#pragma once
#include "Symbols/Variable.h"
#include "Symbols/Function.h"

class Expression;

namespace llvm_utils {
	// Creates a global variable in current llvm::Module
	llvm::Value* createGlobalVariable(
		Variable& var, 
		Expression* initializer
	);

	// Creates a reference to external global variable in module
	llvm::Value* addGlobalVariableFromOtherModule(
		Variable& var, 
		llvm::Module& module
	);

	// Creates a variable for function argument in func's body
	llvm::Value* genFunctionArgumentValue(
		Function* func, 
		const Argument& arg, 
		llvm::Argument* llvmArg
	);

	// Creates a local variable in the beginning of the function
	llvm::Value* createLocalVariable(
		llvm::Function* func, 
		const std::unique_ptr<Type>& type, 
		const std::string& name
	);

	// Creates a value in an unnamed global constant, used for arrays and strings
	llvm::Constant* createGlobalValue(
		llvm::Type* type,
		llvm::Constant* value
	);


	// Returns the default value of the type
	llvm::Constant* getDefaultValueOf(
		const std::unique_ptr<Type>& type
	);

	// Returns the value of the type with all bits in zero
	llvm::Constant* getZeroedValueOf(
		const std::unique_ptr<Type>& type
	);

	// Returns the value of integer type
	llvm::Constant* getConstantInt(
		u64 value, 
		u64 bit_width, 
		bool isSigned = false
	);

	// Returns the value of floating point type
	llvm::Constant* getConstantFP(
		f64 value,
		u64 size
	);

	// Returns the value of string type with stated width in bits (8, 16, 32)
	llvm::Constant* getConstantString(
		const std::string& value,
		u8 symbol_width = 8
	);

	// Returns the value of a struct filled with values
	llvm::Value* getStructValue(
		const std::vector<llvm::Value*> values,
		const std::unique_ptr<Type>& type
	);


	// Converts the value to type -to- from type -from- if it is possible implicitly and returns the converted value
	// If conversion is not possible, return nullptr
	llvm::Value* tryImplicitlyConvertTo(
		const std::unique_ptr<Type>& to, 
		const std::unique_ptr<Type>& from, 
		llvm::Value* value, 
		u64 errLine, 
		bool isFromCompileTime = false // is the value available in compile time
	);

	// Converts the value to type -to- from type -from-. Returns nullptr if the conversion is not possible
	llvm::Value* convertValueTo(
		const std::unique_ptr<Type>& to, 
		const std::unique_ptr<Type>& from, 
		llvm::Value* value
	);

	// Converts the value from the type -from- to some of the three string values (stringType)
	llvm::Value* convertToString(
		const std::unique_ptr<Type>& from, 
		llvm::Value* value, 
		BasicType stringType
	);

	// Converts the value from the type -from- to bool
	llvm::Value* convertToBool(
		const std::unique_ptr<Type>& from, 
		llvm::Value* value
	);
}