#pragma once
#include <memory>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>
#include "llvm/IR/LegacyPassManager.h"

#define FRIEND_CLASS_VISITORS \
	friend class Visitor;

class Visitor;

class INode {
public:


protected:
	int m_errLine;
};

extern llvm::LLVMContext g_context;
extern std::unique_ptr<llvm::IRBuilder<>> g_builder;
extern std::unique_ptr<llvm::legacy::FunctionPassManager> g_functionPassManager;
extern std::unique_ptr<llvm::legacy::PassManager> g_modulePassManager;