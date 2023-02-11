#include "FunctionPrototype.h"
#include <Utils/Defs.h>
#include <Module/Module.h>
#include <Module/LLVMGlobals.h>

FunctionPrototype::FunctionPrototype(const std::string& name, std::unique_ptr<Type> returnType, std::vector<Argument> args,
	FunctionQualities qualities, bool isVaArgs)
	: m_name(name), m_returnType(std::move(returnType)), m_args(std::move(args)), m_qualities(qualities), m_isVaArgs(isVaArgs) {

}

FunctionPrototype::FunctionPrototype(FunctionPrototype& other)
	: m_name(other.m_name), m_returnType(other.m_returnType->copy()), m_qualities(other.m_qualities), m_isVaArgs(other.m_isVaArgs) {
	m_args.reserve(other.m_args.size());
	for (auto& arg : other.m_args) {
		m_args.push_back(Argument{ arg.name, arg.type->copy() });
	}
}

FunctionPrototype::FunctionPrototype(FunctionPrototype&& other) noexcept
	: m_name(std::move(other.m_name)), m_returnType(std::move(other.m_returnType)), m_args(std::move(other.m_args)),
	  m_qualities(other.m_qualities), m_isVaArgs(other.m_isVaArgs) {

}

llvm::Function* FunctionPrototype::generate() const {
	std::vector<llvm::Type*> types;
	for (auto& arg : m_args) {
		types.push_back(arg.type->to_llvm());
	}

	llvm::FunctionType* ft = llvm::FunctionType::get(m_returnType->to_llvm(), types, m_isVaArgs);
	llvm::Function* fun = llvm::Function::Create(ft,
		llvm::Function::ExternalLinkage,
		m_name,
		g_module->getLLVMModule());

	fun->setCallingConv(getCallingConvention(m_qualities.getCallingConvention()));
	fun->setDSOLocal(true);
	
	if (m_qualities.isNoReturn()) {
		fun->addFnAttr(llvm::Attribute::NoReturn);
	}
	
	u32 index = 0;
	for (auto& arg : fun->args())
		arg.setName(m_args[index++].name);

	return fun;
}

llvm::Function* FunctionPrototype::generateImportedFromOtherModule(llvm::Module& thisModule) const {
	std::vector<llvm::Type*> types;
	for (auto& arg : m_args) {
		types.push_back(arg.type->to_llvm());
	}

	llvm::FunctionType* ft = llvm::FunctionType::get(m_returnType->to_llvm(), types, m_isVaArgs);
	llvm::Function* fun = llvm::Function::Create(ft,
		llvm::Function::ExternalLinkage,
		m_name,
		thisModule);

	fun->setCallingConv(getCallingConvention(m_qualities.getCallingConvention()));
	fun->setDSOLocal(true);

	if (m_qualities.isNoReturn()) {
		fun->addFnAttr(llvm::Attribute::NoReturn);
	}

	u32 index = 0;
	for (auto& arg : fun->args())
		arg.setName(m_args[index++].name);

	return fun;
}

const std::string& FunctionPrototype::getName() const {
	return m_name;
}

void FunctionPrototype::setName(const std::string& s) {
	m_name = s;
}

bool FunctionPrototype::isVaArgs() const {
	return m_isVaArgs;
}

void FunctionPrototype::setVaArgs(bool isVaArgs) {
	m_isVaArgs = isVaArgs;
}

FunctionQualities FunctionPrototype::getQualities() const {
	return m_qualities;
}

std::unique_ptr<FunctionType> FunctionPrototype::genType() const {
	return std::make_unique<FunctionType>(std::unique_ptr<Type>(m_returnType->copy()), genArgumentTypes(), m_isVaArgs, false);
}

std::vector<std::unique_ptr<Type>> FunctionPrototype::genArgumentTypes() const {
	std::vector<std::unique_ptr<Type>> argTypes;
	for (auto& arg : m_args) {
		argTypes.push_back(std::unique_ptr<Type>(arg.type->copy()));
	}

	return argTypes;
}

std::vector<Argument>& FunctionPrototype::args() {
	return m_args;
}

const std::unique_ptr<Type>& FunctionPrototype::getReturnType() const {
	return m_returnType;
}

llvm::CallingConv::ID FunctionPrototype::getCallingConvention(CallingConvention conv) {
	switch (conv) {
		case CallingConvention::CCALL: return llvm::CallingConv::C;
		case CallingConvention::STDCALL: return llvm::CallingConv::X86_StdCall;
		case CallingConvention::FASTCALL: return llvm::CallingConv::Fast;
		case CallingConvention::THISCALL: return llvm::CallingConv::X86_ThisCall;
		case CallingConvention::VECTORCALL: return llvm::CallingConv::X86_VectorCall;
		case CallingConvention::COLDCALL: return llvm::CallingConv::Cold;
		case CallingConvention::TAILCALL: return llvm::CallingConv::Tail;
	default: return llvm::CallingConv::C;
	}
}
