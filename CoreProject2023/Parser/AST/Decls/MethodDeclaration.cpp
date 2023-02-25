#include "MethodDeclaration.h"
#include <llvm/IR/Verifier.h>
#include <Parser/Visitor/Visitor.h>
#include <Module/Module.h>
#include <Module/LLVMUtils.h>
#include <Module/LLVMGlobals.h>

extern std::shared_ptr<TypeNode> g_type;


MethodDeclaration::MethodDeclaration(Function* method, std::unique_ptr<Statement> body)
	: m_method(method), m_body(std::move(body)) {

}

void MethodDeclaration::accept(Visitor* visitor, std::unique_ptr<Declaration>& node) {
	visitor->visit(this, node);
}

void MethodDeclaration::generate() {
	if (!m_method->prototype.getQualities().isNative()) {
		llvm::Function* fun = m_method->functionManager->getOriginalValue();
		llvm::BasicBlock* bb = llvm::BasicBlock::Create(g_context, "entry", fun);
		g_builder->SetInsertPoint(bb);

		g_module->addBlock();

		g_function = m_method;
		size_t index = 0;
		for (size_t i = 0; i < m_method->prototype.args().size(); i++) {
			Argument& arg = m_method->prototype.args()[i];
			VariableQualities qualities;
			if (arg.type->isConst) {
				qualities.setVariableType(VariableType::CONST);
			}

			g_module->addLocalVariable(
				arg.name,
				arg.type,
				qualities,
				llvm_utils::genFunctionArgumentValue(
					m_method,
					arg,
					fun->getArg(i)
				)
			);
		}

		if (m_method->prototype.isUsingThisAsArgument()) {
			try {
				m_body->generate();
				if (m_method->prototype.getReturnType()->basicType == BasicType::NO_TYPE) {
					g_builder->CreateRetVoid();
				}
			} catch (TerminatorAdded*) {}
		} else {
			generateConstructor();
		}

		g_module->deleteBlock();
		llvm::verifyFunction(*fun);
		g_functionPassManager->run(*fun, *g_functionAnalysisManager);
	}
}

void MethodDeclaration::generateConstructor() {
	llvm::Function* fun = m_method->functionManager->getOriginalValue();
	llvm::Value* thisVar = llvm_utils::createLocalVariable(fun, m_method->prototype.getReturnType(), "this");

	g_module->addLocalVariable(
		"this",
		m_method->prototype.getReturnType(),
		VariableQualities(),
		thisVar
	);

	try {
		m_body->generate();
	} catch (TerminatorAdded*) {}

	thisVar = g_builder->CreateLoad(m_method->prototype.getReturnType()->to_llvm(), thisVar);
	g_builder->CreateRet(thisVar);
}

std::string MethodDeclaration::toString() const {
	static std::string VISIBILITY_STR[4] = { "local ", "private ", "protected ", "public " };
	static std::string METHOD_TYPE_STR[4] = { "", "static ", "virtual ", "abstract " };

	static std::string MANGLE_STR[2] = { "@mangle\n", "@nomangle\n" };
	static std::string IMPLICIT_STR[2] = { "@implicit\n", "@explicit\n" };
	static std::string SAFETY_STR[3] = { "@unsafe\n", "@safe_only\n", "@safe\n" };
	static std::string CONVENTION_STR[7] = {
		"@ccall\n", "@stdcall\n", "@fastcall\n", "@thiscall\n", "@vectorcall\n", "@coldcall\n", "@tailcall\n"
	};

	std::string result = "";
	FunctionQualities qualities = m_method->prototype.getQualities();
	if (qualities.isNoExcept()) {
		result += "@noexcept\n";
	} if (qualities.isNoReturn()) {
		result += "@noreturn\n";
	} if (qualities.isOverride()) {
		result += "@override\n";
	} if (qualities.getFunctionKind() == FunctionKind::CONSTRUCTOR) {
		result += IMPLICIT_STR[(u8)qualities.isImplicit()];
	}

	result += MANGLE_STR[(u8)qualities.isManglingOn()];
	result += SAFETY_STR[(u8)qualities.getSafety()];
	result += CONVENTION_STR[(u8)qualities.getCallingConvention()];

	result += "def ";

	result += VISIBILITY_STR[(u8)qualities.getVisibility()];
	result += METHOD_TYPE_STR[(u8)qualities.getMethodType()];
	if (qualities.isNative()) {
		result += "native ";
	}

	result += m_method->prototype.getName();

	if (qualities.getFunctionKind() == FunctionKind::OPERATOR) {
		if (m_method->prototype.getName() == "(") {
			result += ')';
		} else if (m_method->prototype.getName() == "[") {
			result += ']';
		}
	}

	result += '(';

	for (auto& arg : m_method->prototype.args()) {
		if (arg.name == "this") {
			continue;
		}

		result += arg.type->toString();
		result += ' ';
		result += arg.name;
		result += ", ";
	}

	if (m_method->prototype.isVaArgs()) {
		result += "...";
	} else {
		result.pop_back();
		result.pop_back();
	}

	result += ") ";

	if (qualities.getFunctionKind() != FunctionKind::CONSTRUCTOR && qualities.getFunctionKind() != FunctionKind::DESTRUCTOR) {
		result += m_method->prototype.getReturnType()->toString();
	}

	if (!qualities.isNative()) {
		result += m_body->toString();
	} else {
		result += ';';
	}

	result += "\n\n";

	return result;
}
