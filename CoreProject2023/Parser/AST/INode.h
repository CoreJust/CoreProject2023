#pragma once
#include <memory>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>

class Visitor;

class INode {
public:


protected:
	int m_errLine;
};

extern llvm::LLVMContext g_context;
extern std::unique_ptr<llvm::IRBuilder<>> g_builder;