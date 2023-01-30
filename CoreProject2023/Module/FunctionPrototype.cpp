#include "FunctionPrototype.h"
#include <Utils/Defs.h>
#include <Parser/AST/INode.h>
#include <Module/Module.h>

FunctionPrototype::FunctionPrototype(const std::string& name, std::vector<Argument> args)
	: m_name(name), m_args(std::move(args)) {

}

llvm::Function* FunctionPrototype::generate(bool is_native) const {
	std::vector<llvm::Type*> types;
	for (auto& arg : m_args) {
		types.push_back(llvm::Type::getInt32Ty(g_context));
	}

	llvm::FunctionType* ft = llvm::FunctionType::get(llvm::Type::getInt32Ty(g_context), types, false);
	llvm::Function* fun = llvm::Function::Create(ft,
		llvm::Function::ExternalLinkage,
		m_name,
		g_module->getLLVMModule());

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

std::vector<Argument>& FunctionPrototype::args() {
	return m_args;
}