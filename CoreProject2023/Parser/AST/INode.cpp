#include "INode.h"

llvm::LLVMContext g_context;
std::unique_ptr<llvm::IRBuilder<>> g_builder = std::make_unique<llvm::IRBuilder<>>(g_context);

// These two are initialized in Compiler::initAll()
std::unique_ptr<llvm::FunctionPassManager> g_functionPassManager;
std::unique_ptr<llvm::ModulePassManager> g_modulePassManager;
std::unique_ptr<llvm::ModuleAnalysisManager> g_moduleAnalysisManager = std::make_unique<llvm::ModuleAnalysisManager>();
std::unique_ptr<llvm::FunctionAnalysisManager> g_functionAnalysisManager = std::make_unique<llvm::FunctionAnalysisManager>();
std::unique_ptr<llvm::LoopAnalysisManager> g_loopAnalysisManager = std::make_unique<llvm::LoopAnalysisManager>();
std::unique_ptr<llvm::CGSCCAnalysisManager> g_cgsccAnalysisManager = std::make_unique<llvm::CGSCCAnalysisManager>();

extern u64* g_errLine;

INode::INode()
	: m_errLine(*g_errLine) {

}

u64 INode::getErrLine() const {
	return m_errLine;
}