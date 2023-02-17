#pragma once
#include <vector>
#include <llvm/IR/Function.h>
#include "FunctionArgument.h"
#include "Annotations.h"

class FunctionPrototype final {
public:
	FunctionPrototype(
		const std::string& name, 
		std::unique_ptr<Type> returnType, 
		std::vector<Argument> args, 
		FunctionQualities qualities, 
		bool isVaArgs
	);

	FunctionPrototype(FunctionPrototype& other);
	FunctionPrototype(FunctionPrototype&& other) noexcept;

	llvm::Function* generate() const;
	llvm::Function* generateImportedFromOtherModule(llvm::Module& thisModule) const;

	// 0 is complete equalness
	// Otherwise - the score showing for how much is the function suitable, the lesser the better
	// A negative value means that the function is not suitable for this arguments at all
	i32 getSuitableness(
		const std::vector<std::unique_ptr<Type>>& argTypes,
		const std::vector<bool>& isCompileTime
	) const;

	// Returns mangled name or just name if the function has @nomangle annotation
	std::string getLLVMName() const;

	std::string genMangledName() const;

	const std::string& getName() const;
	void setName(const std::string& s);

	bool isVaArgs() const;
	void setVaArgs(bool isVaArgs);

	const FunctionQualities& getQualities() const;
	std::unique_ptr<Type>& getReturnType();

	std::unique_ptr<FunctionType> genType() const;

	std::vector<std::unique_ptr<Type>> genArgumentTypes() const;
	std::vector<Argument>& args();
	const std::unique_ptr<Type>& getReturnType() const;

	// True for a method and destructor
	bool isUsingThisAsArgument() const;

	// True for a method, destructor and constructor
	bool isUsingThis() const;

	std::string toString() const;

	static llvm::CallingConv::ID getCallingConvention(CallingConvention conv);

private:
	std::string m_name;
	std::unique_ptr<Type> m_returnType;
	std::vector<Argument> m_args;
	FunctionQualities m_qualities;
	bool m_isVaArgs;
};