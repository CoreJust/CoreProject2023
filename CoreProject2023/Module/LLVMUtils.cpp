#include "LLVMUtils.h"
#include <llvm/IR/GlobalVariable.h>
#include <llvm/Transforms/Utils/ModuleUtils.h>
#include <Parser/AST/INode.h>
#include "Module.h"
#include "LLVMGlobals.h"
#include <Utils/ErrorManager.h>
#include <Parser/AST/Exprs/Expression.h>

llvm::Value* llvm_utils::createGlobalVariable(Variable& var, Expression* initializer) {
	VariableType varType = var.qualities.getVariableType();
	bool isConst = varType == VariableType::CONST;
	bool isExternal = varType == VariableType::EXTERN;
	bool isConstructorNeeded = false;

	Expression* init = (Expression*)initializer;

	// Generating the value by default
	llvm::Constant* defaultVal = nullptr;
	if (init != nullptr && init->isCompileTime()) {
		defaultVal = (llvm::Constant*)init->generate();
		defaultVal = (llvm::Constant*)llvm_utils::tryImplicitlyConvertTo(
			var.type, // resulting type
			init->getType(), // initializing type
			defaultVal,  // initializing valie
			init->getErrLine(), // the line in the original file
			init->isCompileTime() // is the value compile-time
		);
	} else if (!isExternal) {
		defaultVal = getDefaultValueOf(var.type);
		isConstructorNeeded = true;
	}
	
	// Creating the variable
	llvm::GlobalValue::ThreadLocalMode threadLocalMode = var.qualities.isThreadLocal() ?
		llvm::GlobalValue::GeneralDynamicTLSModel :
		llvm::GlobalValue::NotThreadLocal;

	llvm::GlobalVariable* varValue = new llvm::GlobalVariable(
		g_module->getLLVMModule(), // current llvm::Module
		var.type->to_llvm(), // variable type
		!isConstructorNeeded && isConst, // is constant
		llvm::GlobalValue::LinkageTypes::ExternalLinkage, // linkage
		defaultVal, // default value
		var.name, // variable name
		nullptr, // insert before
		threadLocalMode, // is it thread-local
		0, // address space
		isExternal // is externally initialized
	);

	if (isExternal) {
		varValue->addAttribute("dso_local");
	}

	// Creating variable's global constructore if needed
	if (!isExternal && init != nullptr && isConstructorNeeded) {
		// Creating global constructor function
		llvm::FunctionType* initFuncType = llvm::FunctionType::get(llvm::Type::getVoidTy(g_context), false);
		llvm::Function* initFunc = llvm::Function::Create(
			initFuncType, 
			llvm::Function::ExternalLinkage,
			"$init_" + var.name, 
			g_module->getLLVMModule()
		);

		// Adding initializing code to the constructor
		llvm::BasicBlock* bb = llvm::BasicBlock::Create(g_context, "entry", initFunc);
		g_builder->SetInsertPoint(bb);

		llvm::Value* val = init->generate();
		val = (llvm::Constant*)llvm_utils::tryImplicitlyConvertTo(var.type, init->getType(), val, init->getErrLine());

		g_builder->CreateStore(val, varValue, var.qualities.getVisibility() != Visibility::PRIVATE);
		g_builder->CreateRetVoid();

		// Adding the constructor to module's global constructor list
		llvm::appendToGlobalCtors(g_module->getLLVMModule(), initFunc, 1);
	}

	var.value = varValue;
	return varValue;
}

llvm::Value* llvm_utils::addGlobalVariableFromOtherModule(Variable& var, llvm::Module& module) {
	llvm::GlobalValue::ThreadLocalMode threadLocalMode = var.qualities.isThreadLocal() ?
		llvm::GlobalValue::GeneralDynamicTLSModel :
		llvm::GlobalValue::NotThreadLocal;
	llvm::GlobalVariable* varValue = new llvm::GlobalVariable(
		module, // the module where the variable is added to
		var.type->to_llvm(), // variable type
		false, // is constant
		llvm::GlobalValue::LinkageTypes::ExternalLinkage, // linkage
		nullptr, // initializer
		var.name, // variable name
		nullptr, // insert before
		threadLocalMode, // thread-local mode
		0, // address space
		true // is externally initialized
	);

	varValue->addAttribute("dso_local");
	var.value = varValue;
	return varValue;
}

llvm::Value* llvm_utils::genFunctionArgumentValue(
	Function* func, 
	const Argument& arg, 
	llvm::Argument* llvmArg
) {
	llvm::Value* result = createLocalVariable(func->functionValue, arg.type, arg.name);
	g_builder->CreateStore(llvmArg, result);
	return result;

}

llvm::Value* llvm_utils::createLocalVariable(
	llvm::Function* func, 
	const std::unique_ptr<Type>& type, 
	const std::string& name
) {
	llvm::IRBuilder<> tmpBuilder(&func->getEntryBlock(), func->getEntryBlock().begin());
	return tmpBuilder.CreateAlloca(type->to_llvm(), 0, name);
}

llvm::Constant* llvm_utils::createGlobalValue(llvm::Type* type, llvm::Constant* value) {
	llvm::GlobalVariable* globalValue = new llvm::GlobalVariable(
		g_module->getLLVMModule(), // current llvm::Module
		type, // gloval value type
		false, // is constant
		llvm::GlobalValue::LinkageTypes::InternalLinkage, // linkage
		value, // default value
		"global$", // variable name
		nullptr, // insert before
		llvm::GlobalValue::NotThreadLocal, // is it thread-local
		0, // address space
		false // is externally initialized
	);

	return globalValue;
}

llvm::Constant* llvm_utils::getDefaultValueOf(const std::unique_ptr<Type>& type) {
	llvm::Type* llvmType = type->to_llvm();
	switch (type->basicType) {
		case BasicType::NO_TYPE: return nullptr;
		case BasicType::I8: return getConstantInt(0, 8, true);
		case BasicType::I16: return getConstantInt(0, 16, true);
		case BasicType::I32: return getConstantInt(0, 32, true);
		case BasicType::I64: return getConstantInt(0, 64, true);
		case BasicType::U8: return getConstantInt(0, 8, false);
		case BasicType::U16: return getConstantInt(0, 16, false);
		case BasicType::U32: return getConstantInt(0, 32, false);
		case BasicType::U64: return getConstantInt(0, 64, false);
		case BasicType::F32: return getConstantFP(0, 32);
		case BasicType::F64: return getConstantFP(0, 64);
		case BasicType::BOOL: return getConstantInt(0, 1, false);
		case BasicType::C8: return getConstantInt(0, 8, true);
		case BasicType::C16: return getConstantInt(0, 16, true);
		case BasicType::C32: return getConstantInt(0, 32, true);
		case BasicType::STR8: return getConstantString("", 8);
		case BasicType::STR16: return getConstantString("", 16);
		case BasicType::STR32: return getConstantString("", 32);
		case BasicType::ARRAY: {
			std::vector<llvm::Constant*> init;
			ArrayType* arrType = (ArrayType*)type.get();
			init.reserve(arrType->size);
			for (size_t i = 0; i < arrType->size; i++) {
				init.push_back(getDefaultValueOf(arrType->elementType));
			}

			return llvm::ConstantArray::get((llvm::ArrayType*)llvmType, init);
		};
		case BasicType::DYN_ARRAY:
			return llvm::ConstantStruct::get(
				(llvm::StructType*)llvmType,
				llvm::ArrayRef<llvm::Constant*>({ // default value: { null, 0 }
					llvm::ConstantPointerNull::get(
						llvm::PointerType::get(((PointerType*)type.get())->elementType->to_llvm(), 0)
					),
					getConstantInt(0, 64, false)
				})
			);
		case BasicType::FUNCTION:
		case BasicType::POINTER: return llvm::ConstantPointerNull::get((llvm::PointerType*)llvmType);
		case BasicType::REFERENCE: return nullptr; // no default value
		case BasicType::RVAL_REFERENCE: return nullptr; // no default value
		case BasicType::OPTIONAL:
			return llvm::ConstantStruct::get(
				(llvm::StructType*)llvmType,
				llvm::ArrayRef<llvm::Constant*>({ // default value: { 0 as type, false }
					getZeroedValueOf(((PointerType*)type.get())->elementType),
					getConstantInt(0, 1, false)
				})
			);
		case BasicType::TUPLE: {
			std::vector<llvm::Constant*> init;
			TupleType* tupType = (TupleType*)type.get();
			init.reserve(tupType->subTypes.size());
			for (auto& subType : tupType->subTypes) {
				init.push_back(getDefaultValueOf(subType));
			}

			return llvm::ConstantStruct::get((llvm::StructType*)llvmType, init);
		};
		case BasicType::STRUCT: {
			std::vector<llvm::Constant*> init;
			StructType* structType = (StructType*)type.get();
			init.reserve(structType->fieldTypes.size());
			for (auto& fieldType : structType->fieldTypes) {
				init.push_back(getDefaultValueOf(fieldType));
			}

			return llvm::ConstantStruct::get((llvm::StructType*)llvmType, init);
		};
		case BasicType::TYPE_NODE: return getDefaultValueOf(((TypeNodeType*)type.get())->node->type);
	default: break;
	}

	return nullptr;
}

llvm::Constant* llvm_utils::getZeroedValueOf(const std::unique_ptr<Type>& type) {
	llvm::Type* llvmType = type->to_llvm();
	llvm::Constant* value = getConstantInt(
		0, 
		g_module->getLLVMModule().getDataLayout().getTypeAllocSizeInBits(llvmType)
	);

	return (llvm::Constant*)g_builder->CreateBitCast(value, llvmType);
}

llvm::Constant* llvm_utils::getConstantInt(
	u64 value, 
	u64 bit_width, 
	bool isSigned
) {
	return llvm::ConstantInt::get(g_context, llvm::APInt(bit_width, value, isSigned));
}

llvm::Constant* llvm_utils::getConstantFP(f64 value, u64 size) {
	if (size == 32) {
		return llvm::ConstantFP::get(g_context, llvm::APFloat(float(value)));
	} else {
		return llvm::ConstantFP::get(g_context, llvm::APFloat(value));
	}
}

llvm::Constant* llvm_utils::getConstantString(
	const std::string& value, 
	u8 symbol_width
) {
	ASSERT(symbol_width == 8 || symbol_width == 16 || symbol_width == 32, "Impossible symbol width");

	if (symbol_width == 8) {
		llvm::Constant* llvmValue = llvm::ConstantDataArray::getString(g_context, value);
		llvmValue = createGlobalValue(llvmValue->getType(), llvmValue);

		return llvm::ConstantStruct::get(
			(llvm::StructType*)basicTypeToLLVM(BasicType::STR8), // string type (struct { c8* data, u64 size })
			llvm::ArrayRef<llvm::Constant*>({ // string value
				llvmValue,
				getConstantInt(value.size(), 64, false)
			})
		);
	} else {
		u8 symbolByteWidth = symbol_width / 8;
		ASSERT(value.size() % symbolByteWidth == 0, "Incorrect value");

		// Getting the value as u16/u32 array
		std::vector<llvm::Constant*> buffer;
		for (size_t i = 0; i < value.size(); i += symbolByteWidth) {
			if (symbol_width == 16) {
				buffer.push_back(getConstantInt(*(i16*)&value[i], 16, true));
			} else { // symbol_width == 32
				buffer.push_back(getConstantInt(*(i32*)&value[i], 32, true));
			}
		}

		buffer.push_back(getConstantInt(0, symbol_width, true)); // terminating zero

		llvm::Constant* llvmValue = llvm::ConstantArray::get(
			llvm::ArrayType::get(llvm::Type::getIntNTy(g_context, symbol_width), value.size() / symbolByteWidth + 1),
			buffer
		);

		llvmValue = createGlobalValue(llvmValue->getType(), llvmValue);

		if (symbol_width == 16) {
			return llvm::ConstantStruct::get(
				(llvm::StructType*)basicTypeToLLVM(BasicType::STR16), // string type (struct { c16* data, u64 size })
				llvm::ArrayRef<llvm::Constant*>({ // string value
					llvmValue,
					getConstantInt(value.size() / 2, 64, false)
					})
			);
		} else { // symbol_width == 32
			return llvm::ConstantStruct::get(
				(llvm::StructType*)basicTypeToLLVM(BasicType::STR32), // string type (struct { c32* data, u64 size })
				llvm::ArrayRef<llvm::Constant*>({ // string value
					llvmValue,
					getConstantInt(value.size() / 4, 64, false)
					})
			);
		}
	}
}

llvm::Value* llvm_utils::tryImplicitlyConvertTo(
	const std::unique_ptr<Type>& to, 
	const std::unique_ptr<Type>& from,
	llvm::Value* value, 
	u64 errLine, 
	bool isFromCompileTime
) {
	if (isImplicitlyConverible(from, to, isFromCompileTime)) {
		return llvm_utils::convertValueTo(to, from, value);
	} else {
		ErrorManager::typeError(ErrorID::E3101_CANNOT_BE_IMPLICITLY_CONVERTED, errLine,
			from->toString() + " to " + to->toString());
		return nullptr;
	}
}

llvm::Value* llvm_utils::convertValueTo(
	const std::unique_ptr<Type>& to, 
	const std::unique_ptr<Type>& from, 
	llvm::Value* value
) {
	if (to->equalsOrLessConstantThan(from) >= -4096) { // equals not considering constantness
		return value;
	}

	BasicType bfrom = from->basicType;
	BasicType bto = to->basicType;

	if (bto == BasicType::NO_TYPE) {
		return nullptr;
	}

	if (bfrom == bto && bfrom <= BasicType::STR32) {
		return value;
	}

	// For integral (non-user-defined) types
	if (isReference(bfrom)) {
		return convertValueTo(to, ((PointerType*)from.get())->elementType, value);
	} else if (isReference(bto)) {
		return convertValueTo(((PointerType*)to.get())->elementType, from, value);
	}

	if (bfrom == BasicType::TYPE_NODE && ((TypeNodeType*)from.get())->node->type->basicType < BasicType::CLASS) {
		return convertValueTo(to, ((TypeNodeType*)from.get())->node->type, value);
	} else if (bto == BasicType::TYPE_NODE && ((TypeNodeType*)to.get())->node->type->basicType < BasicType::CLASS) {
		return convertValueTo(((TypeNodeType*)to.get())->node->type, from, value);
	}

	if (bto == BasicType::BOOL) {
		return convertToBool(from, value);
	}

	if (isString(bto)) {
		return convertToString(from, value, bto);
	}

	if (isInteger(bfrom) || isChar(bfrom)) {
		if (isInteger(bto) || isChar(bto)) {
			return g_builder->CreateIntCast(value, to->to_llvm(), isSigned(bto) || isChar(bto));
		} else if (isFloat(bto)) {
			if (isSigned(bfrom) || isChar(bfrom)) {
				return g_builder->CreateSIToFP(value, to->to_llvm());
			} else {
				return g_builder->CreateUIToFP(value, to->to_llvm());
			}
		} else if (bto == BasicType::POINTER || bto == BasicType::FUNCTION || bto == BasicType::ARRAY) {
			return g_builder->CreateIntToPtr(value, to->to_llvm());
		}
	} else if (isFloat(bfrom)) {
		if (isFloat(bto)) {
			return g_builder->CreateFPCast(value, to->to_llvm());
		} else if (isInteger(bto) || isChar(bto)) {
			if (isSigned(bto) || isChar(bto)) {
				return g_builder->CreateFPToSI(value, to->to_llvm());
			} else {
				return g_builder->CreateFPToUI(value, to->to_llvm());
			}
		}
	} else if (bfrom == BasicType::BOOL) {
		if (isInteger(bto) || isChar(bto)) {
			return g_builder->CreateZExt(value, to->to_llvm());
		}
	} else if (bfrom == BasicType::POINTER || bfrom == BasicType::FUNCTION || bfrom == BasicType::ARRAY) {
		if (bto == BasicType::POINTER || bto == BasicType::FUNCTION || bto == BasicType::ARRAY) {
			return g_builder->CreateBitCast(value, to->to_llvm());
		} else if (isInteger(bto) || isChar(bto)) {
			value = g_builder->CreatePtrToInt(value, llvm::Type::getInt64Ty(g_context));
			return g_builder->CreateIntCast(value, to->to_llvm(), isSigned(bto) || isChar(bto));
		}
	} else if (isString(bfrom)) {
		if (bto == BasicType::POINTER) {
			return g_builder->CreateExtractValue(value, llvm::ArrayRef<u32>(0)); // getting .data
		}
	}

	if (bfrom == BasicType::STRUCT && bto == BasicType::TUPLE) {
		if (((StructType*)from.get())->isEquivalentTo(((TupleType*)to.get())->subTypes)) {
			return value;
		}
	} else if (bfrom == BasicType::TUPLE && bto == BasicType::STRUCT) {
		if (((TupleType*)from.get())->isEquivalentTo(((StructType*)to.get())->fieldTypes)) {
			return value;
		}
	}

	std::vector<std::unique_ptr<Type>> types;
	types.push_back(from->copy());
	if (Function* constructor = g_module->chooseConstructor(to, types, { true }, true)) {
		value = convertValueTo(constructor->prototype.args()[0].type, from, value);

		return g_builder->CreateCall(
			(llvm::FunctionType*)constructor->prototype.genType()->to_llvmFunctionType(),
			constructor->functionValue,
			{ value }
		);
	}

	return nullptr;
}

llvm::Value* llvm_utils::convertToString(const std::unique_ptr<Type>& from, llvm::Value* value, BasicType stringType) {
	TypeNode& stringNode = getBasicTypeNode(stringType);
	// TODO: implement

	return nullptr;
}

llvm::Value* llvm_utils::convertToBool(const std::unique_ptr<Type>& from, llvm::Value* value) {
	if (from->basicType == BasicType::BOOL) {
		return value;
	}

	if (isReference(from->basicType)) {
		return convertToBool(((PointerType*)from.get())->elementType, value);
	}

	if (isInteger(from->basicType) || isChar(from->basicType)) {
		return g_builder->CreateICmpNE(value, getDefaultValueOf(from));
	} else if (isFloat(from->basicType)) {
		return g_builder->CreateFCmpUNE(value, getDefaultValueOf(from));
	} else if (from->basicType == BasicType::POINTER || from->basicType == BasicType::FUNCTION) {
		value = g_builder->CreateBitCast(value, llvm::Type::getInt64Ty(g_context));
		llvm::Value* null = llvm::ConstantPointerNull::get((llvm::PointerType*)from->to_llvm());
		null = g_builder->CreateIntToPtr(null, llvm::Type::getInt64Ty(g_context));
		return g_builder->CreateICmpNE(value, null);
	} else if (isString(from->basicType) || from->basicType == BasicType::DYN_ARRAY) {
		value = g_builder->CreateExtractValue(value, { 1 }); // getting .size
		return g_builder->CreateICmpNE(value, getConstantInt(0, 64, false));
	} else if (from->basicType == BasicType::OPTIONAL) {
		value = g_builder->CreateExtractValue(value, { 1 }); // getting .has
		return g_builder->CreateICmpNE(value, getConstantInt(0, 64, false));
	}

	return nullptr;
}
