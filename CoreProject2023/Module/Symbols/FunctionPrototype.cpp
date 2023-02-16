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
		getLLVMName(),
		g_module->getLLVMModule()
	);

	fun->setCallingConv(getCallingConvention(m_qualities.getCallingConvention()));
	fun->setDSOLocal(true);
	
	if (m_qualities.isNoReturn()) {
		fun->addFnAttr(llvm::Attribute::NoReturn);
	}
	
	i32 index = 0;
	for (auto& arg : fun->args()) {
		arg.setName(m_args[index++].name);
	}

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
		getLLVMName(),
		thisModule);

	fun->setCallingConv(getCallingConvention(m_qualities.getCallingConvention()));
	fun->setDSOLocal(true);

	if (m_qualities.isNoReturn()) {
		fun->addFnAttr(llvm::Attribute::NoReturn);
	}

	i32 index = 0;
	for (auto& arg : fun->args()) {
		arg.setName(m_args[index++].name);
	}

	return fun;
}

i32 FunctionPrototype::getSuitableness(
	const std::vector<std::unique_ptr<Type>>& argTypes,
	const std::vector<bool>& isCompileTime
) const {
	auto isCT = [&](size_t i) -> bool {
		if (i >= isCompileTime.size()) {
			return false;
		} else {
			return isCompileTime[i];
		}
	};

	// TODO: add consideration for default values of arguments
	i32 result = 0;
	if (m_isVaArgs && m_args.size() < argTypes.size()) {
		result += 80192;
	}

	if (m_args.size() != argTypes.size() && !result) {
		return -1;
	}

	for (size_t i = 0; i < m_args.size(); i++) {
		if (i32 convertibility = evaluateConvertibility(argTypes[i], m_args[i].type, isCT(i));
			convertibility >= 0 ) {
			result += convertibility;
		} else {
			return -1;
		}
	}

	return result;
}

std::string FunctionPrototype::getLLVMName() const {
	if (m_qualities.isManglingOn() && m_name != "main") {
		return genMangledName();
	} else {
		return m_name;
	}
}

std::string FunctionPrototype::genMangledName() const {
	std::string result = m_name + "$" + m_returnType->toMangleString();
	
	for (auto& arg : m_args) {
		result.append("_").append(arg.type->toMangleString());
	}

	return result;
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

FunctionQualities& FunctionPrototype::getQualities() {
	return m_qualities;
}

std::unique_ptr<Type>& FunctionPrototype::getReturnType() {
	return m_returnType;
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

bool FunctionPrototype::isUsingThisAsArgument() const {
	return m_qualities.isMethod() || m_qualities.getFunctionKind() == FunctionKind::DESTRUCTOR;
}

bool FunctionPrototype::isUsingThis() const {
	return isUsingThisAsArgument() || m_qualities.getFunctionKind() == FunctionKind::CONSTRUCTOR;
}

std::string FunctionPrototype::toString() const {
	std::string result = m_name;
	result += '<';
	for (auto& arg : m_args) {
		result += arg.type->toString();
		result += ' ';
		result += arg.name;
		result += ", ";
	}

	result.pop_back();
	result.back() = '>';

	return result;
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
