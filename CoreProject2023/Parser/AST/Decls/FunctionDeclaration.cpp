#include "FunctionDeclaration.h"
#include "llvm/IR/Verifier.h"
#include <Parser/Visitor/Visitor.h>
#include <Module/Module.h>
#include <Module/LLVMUtils.h>
#include <Module/LLVMGlobals.h>

Function* g_function;

FunctionDeclaration::FunctionDeclaration(Function* func, std::unique_ptr<Statement> body)
	: m_function(func), m_body(std::move(body)) {

}

void FunctionDeclaration::accept(Visitor* visitor, std::unique_ptr<Declaration>& node) {
	visitor->visit(this, node);
}

void FunctionDeclaration::generate() {
	if (!m_function->prototype.getQualities().isNative()) {
		llvm::Function* fun = m_function->functionManager->getOriginalValue();
		llvm::BasicBlock* bb = llvm::BasicBlock::Create(g_context, "entry", fun);
		g_builder->SetInsertPoint(bb);

		g_module->addBlock();
		g_function = m_function;
		size_t index = 0;
		for (size_t i = 0; i < m_function->prototype.args().size(); i++) {
			Argument& arg = m_function->prototype.args()[i];
			VariableQualities qualities;
			if (arg.type->isConst) {
				qualities.setVariableType(VariableType::CONST);
			}

			g_module->addLocalVariable(
				arg.name,
				arg.type,
				qualities,
				llvm_utils::genFunctionArgumentValue(
					m_function,
					arg,
					fun->getArg(i)
				)
			);
		}

		if (m_function->prototype.isUsingThis()) {
			if (m_function->prototype.isUsingThisAsArgument()) {
				// Methods and destructor, shall not be called
			} else {
				generateConstructor();
			}
		} else {
			try {
				m_body->generate();
				if (m_function->prototype.getReturnType()->basicType == BasicType::NO_TYPE) {
					g_builder->CreateRetVoid();
				}
			} catch (TerminatorAdded*) {}
		}

		g_module->deleteBlock();
		llvm::verifyFunction(*fun);
		g_functionPassManager->run(*fun, *g_functionAnalysisManager);
	}
}

void FunctionDeclaration::generateConstructor() {
	llvm::Function* fun = m_function->functionManager->getOriginalValue();
	llvm::Value* thisVar = llvm_utils::createLocalVariable(fun, m_function->prototype.getReturnType(), "this");

	g_module->addLocalVariable(
		"this",
		m_function->prototype.getReturnType(),
		VariableQualities(),
		thisVar
	);

	try {
		m_body->generate();
	} catch (TerminatorAdded*) { }

	thisVar = g_builder->CreateLoad(m_function->prototype.getReturnType()->to_llvm(), thisVar);
	g_builder->CreateRet(thisVar);
}

std::string FunctionDeclaration::toString() const {
	static std::string VISIBILITY_STR[4] = { "@local\n", "@private\n", "@direct_import\n", "@public\n" };
	static std::string MANGLE_STR[2] = { "@mangle\n", "@nomangle\n" };
	static std::string IMPLICIT_STR[2] = { "@implicit\n", "@explicit\n" };
	static std::string SAFETY_STR[3] = { "@unsafe\n", "@safe_only\n", "@safe\n" };
	static std::string CONVENTION_STR[7] = {
		"@ccall\n", "@stdcall\n", "@fastcall\n", "@thiscall\n", "@vectorcall\n", "@coldcall\n", "@tailcall\n"
	};

	std::string result = "";
	FunctionQualities qualities = m_function->prototype.getQualities();
	if (qualities.isNoExcept()) {
		result += "@noexcept\n";
	} if (qualities.isNoReturn()) {
		result += "@noreturn\n";
	} if (qualities.getFunctionKind() == FunctionKind::CONSTRUCTOR) {
		result += IMPLICIT_STR[(u8)qualities.isImplicit()];
	}

	result += MANGLE_STR[(u8)qualities.isManglingOn()];
	result += SAFETY_STR[(u8)qualities.getSafety()];
	result += CONVENTION_STR[(u8)qualities.getCallingConvention()];
	result += VISIBILITY_STR[(u8)qualities.getVisibility()];

	result += "def ";

	if (qualities.isNative()) {
		result += "native ";
	}

	result += m_function->prototype.getName();

	if (qualities.getFunctionKind() == FunctionKind::OPERATOR) {
		if (m_function->prototype.getName() == "(") {
			result += ')';
		} else if (m_function->prototype.getName() == "[") {
			result += ']';
		}
	}

	result += '(';

	for (auto& arg : m_function->prototype.args()) {
		result += arg.type->toString();
		result += ' ';
		result += arg.name;
		result += ", ";
	}

	if (m_function->prototype.isVaArgs()) {
		result += "...";
	} else {
		result.pop_back();
		result.pop_back();
	}

	result += ") ";

	if (qualities.getFunctionKind() != FunctionKind::CONSTRUCTOR && qualities.getFunctionKind() != FunctionKind::DESTRUCTOR) {
		result += m_function->prototype.getReturnType()->toString();
	}

	if (!qualities.isNative()) {
		result += m_body->toString();
	} else {
		result += ';';
	}

	result += "\n\n";

	return result;
}
