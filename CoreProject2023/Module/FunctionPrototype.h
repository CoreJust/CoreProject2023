#pragma once
#include <vector>
#include <llvm/IR/Function.h>
#include "FunctionArgument.h"
#include "Annotations.h"

class FunctionPrototype final {
public:
	FunctionPrototype(const std::string& name, std::unique_ptr<Type> returnType, std::vector<Argument> args);
	FunctionPrototype(FunctionPrototype& other);
	FunctionPrototype(FunctionPrototype&& other);

	llvm::Function* generate(bool is_native, CallingConvention conv = CallingConvention::FASTCALL) const;
	llvm::Function* generateImportedFromOtherModule(llvm::Module& thisModule, CallingConvention conv = CallingConvention::FASTCALL) const;

	const std::string& getName() const;
	void setName(const std::string& s);

	std::unique_ptr<FunctionType> genType() const;

	std::vector<std::unique_ptr<Type>> genArgumentTypes() const;
	std::vector<Argument>& args();

	static llvm::CallingConv::ID getCallingConvention(CallingConvention conv);

private:
	std::string m_name;
	std::unique_ptr<Type> m_returnType;
	std::vector<Argument> m_args;
};