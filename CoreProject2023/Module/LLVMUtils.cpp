#include "LLVMUtils.h"
#include <llvm/IR/GlobalVariable.h>
#include <llvm/Transforms/Utils/ModuleUtils.h>
#include <Parser/AST/INode.h>
#include "Module.h"

llvm::Value* llvm_utils::createGlobalVariable(Variable& var, llvm::Value* initializer) {
	VariableType varType = var.qualities.getVariableType();
	bool isConst = varType == VariableType::CONST;
	bool isExternal = varType == VariableType::EXTERN;
	bool isConstructorNeeded = false;

	llvm::Constant* defaultVal = nullptr;
	if (initializer != nullptr && llvm::isa<llvm::Constant>(initializer)) {
		defaultVal = (llvm::Constant*)initializer;
	} else if (!isExternal) {
		defaultVal = getDefaultValueOf(var.type);
		isConstructorNeeded = true;
	}
	
	llvm::GlobalVariable* varValue = new llvm::GlobalVariable(g_module->getLLVMModule(),
		var.type->to_llvm(), false, llvm::GlobalValue::LinkageTypes::ExternalLinkage,
		defaultVal, var.name, nullptr, llvm::GlobalValue::NotThreadLocal, 0, isExternal);

	if (isExternal) {
		varValue->addAttribute("dso_local");
	}

	if (!isExternal && initializer != nullptr && isConstructorNeeded) {
		llvm::FunctionType* initFuncType = llvm::FunctionType::get(llvm::Type::getVoidTy(g_context), false);
		llvm::Function* initFunc = llvm::Function::Create(initFuncType, llvm::Function::ExternalLinkage,
			"$init_" + var.name, g_module->getLLVMModule());
		llvm::BasicBlock* bb = llvm::BasicBlock::Create(g_context, "entry", initFunc);
		g_builder->SetInsertPoint(bb);

		g_builder->CreateStore(initializer, varValue, var.qualities.getVisibility() != Visibility::PRIVATE);
		g_builder->CreateRetVoid();

		llvm::appendToGlobalCtors(g_module->getLLVMModule(), initFunc, 1);
	}

	var.value = varValue;
	return varValue;
}

llvm::Value* llvm_utils::addGlobalVariableFromOtherModule(Variable& var, llvm::Module& module) {
	llvm::GlobalVariable* varValue = new llvm::GlobalVariable(module,
		llvm::Type::getInt32Ty(g_context), false, llvm::GlobalValue::LinkageTypes::ExternalLinkage,
		nullptr, var.name, nullptr, llvm::GlobalValue::NotThreadLocal, 0, true);

	varValue->addAttribute("dso_local");
	var.value = varValue;
	return varValue;
}

llvm::Value* llvm_utils::genFunctionArgumentValue(Function* func, Argument& arg, llvm::Argument* llvmArg) {
	llvm::Value* result = createLocalVariable(func->functionValue, arg.type, arg.name);
	g_builder->CreateStore(llvmArg, result);
	return result;

}

llvm::Value* llvm_utils::createLocalVariable(llvm::Function* func, const std::unique_ptr<Type>& type, const std::string& name) {
	llvm::IRBuilder<> tmpBuilder(&func->getEntryBlock(), func->getEntryBlock().begin());
	return tmpBuilder.CreateAlloca(type->to_llvm(), 0, name);
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
		case BasicType::F32: return llvm::ConstantFP::get(g_context, llvm::APFloat(0.f));
		case BasicType::F64: return llvm::ConstantFP::get(g_context, llvm::APFloat(0.0));
		case BasicType::BOOL: return getConstantInt(0, 1, false);
		case BasicType::C8: return getConstantInt(0, 8, true);
		case BasicType::C16: return getConstantInt(0, 16, true);
		case BasicType::C32: return getConstantInt(0, 32, true);
		case BasicType::STR8: return llvm::ConstantStruct::get((llvm::StructType*)llvmType,
				llvm::ArrayRef<llvm::Constant*>({
					llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(g_context)), getConstantInt(0, 64, false)
				}));
		case BasicType::STR16:
			return llvm::ConstantStruct::get(
				(llvm::StructType*)llvmType,
				llvm::ArrayRef<llvm::Constant*>({
					llvm::ConstantPointerNull::get(llvm::Type::getInt16PtrTy(g_context)),
					getConstantInt(0, 64, false)
				})
			);
		case BasicType::STR32:
			return llvm::ConstantStruct::get(
				(llvm::StructType*)llvmType,
				llvm::ArrayRef<llvm::Constant*>({
					llvm::ConstantPointerNull::get(llvm::Type::getInt32PtrTy(g_context)),
					getConstantInt(0, 64, false)
				})
			);
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
				llvm::ArrayRef<llvm::Constant*>({
					llvm::ConstantPointerNull::get((llvm::PointerType*)(((llvm::StructType*)llvmType)->elements()[0])),
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
				llvm::ArrayRef<llvm::Constant*>({
					getDefaultValueOf(((PointerType*)type.get())->elementType),
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
			// TODO: implement for user-defined types
	default: break;
	}
}

llvm::Constant* llvm_utils::getConstantInt(u64 value, u64 bit_width, bool isSigned) {
	return llvm::ConstantInt::get(g_context, llvm::APInt(bit_width, value, isSigned));
}

llvm::Value* llvm_utils::convertValueTo(const std::unique_ptr<Type>& to, const std::unique_ptr<Type>& from, llvm::Value* value) {
	BasicType bfrom = from->basicType;
	BasicType bto = to->basicType;

	// TODO: user-defined types

	// For integral (non-user-defined) types
	if (isReference(from->basicType)) {
		if (from->basicType == BasicType::REFERENCE) {
			value = g_builder->CreateLoad(from->to_llvm(), value);
		}

		return convertValueTo(to, ((PointerType*)from.get())->elementType, value);
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
			return g_builder->CreateIntCast(value, to->to_llvm(), isSigned(bto) || isChar(bto));
		}
	} else if (bfrom == BasicType::POINTER || bfrom == BasicType::FUNCTION || bfrom == BasicType::ARRAY) {
		if (bto == BasicType::POINTER || bto == BasicType::FUNCTION || bto == BasicType::ARRAY) {
			return g_builder->CreateBitCast(value, to->to_llvm());
		} else if (isInteger(bto) || isChar(bto)) {
			value = g_builder->CreateBitCast(value, llvm::Type::getInt64Ty(g_context));
			return g_builder->CreateIntCast(value, to->to_llvm(), isSigned(bto) || isChar(bto));
		}
	} else if (isString(bfrom)) {
		if (bto == BasicType::POINTER) {
			return g_builder->CreateExtractValue(value, llvm::ArrayRef<u32>(0)); // getting .data
		}
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
		if (from->basicType == BasicType::REFERENCE) {
			value = g_builder->CreateLoad(from->to_llvm(), value);
		}

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
}
