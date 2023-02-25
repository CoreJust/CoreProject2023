#include "ProjectSettings.h"
#include <llvm/Support/Host.h>
#include <Utils/String.h>

CompilerOutput::CompilerOutput() {
	setOutput(Lexer, NoOut);
	setOutput(ASTBeforeOpt, NoOut);
	setOutput(ASTAfterOpt, NoOut);
	setOutput(IRBeforeOpt, NoOut);
	setOutput(IRAfterOpt, NoOut);

	setOutput(ObjectData, File);
	setOutput(ExecutableData, File);
}

void CompilerOutput::setOutput(OutputStage stage, OutputMode mode, std::string outputFile) {
	outputModes[stage] = mode;
	outputFiles[stage] = outputFile;
}

CompilerOutput::OutputMode CompilerOutput::getOutputMode(OutputStage stage) {
	return outputModes[stage];
}

std::string CompilerOutput::getOutputFile(OutputStage stage) {
	return outputFiles[stage];
}

ProjectSettings::ProjectSettings()
 :
	configuration(Configuration::Debug),
	optLevel(OptimizationLevel::O2),
	compilationMode(CompilationMode::Program) {
	std::vector<std::string> triple = split(llvm::sys::getDefaultTargetTriple(), '-');

	if (triple.size()) {
		targetArch = triple[0];
	} else {
		targetArch = "x86_64";
	}

	if (triple.size() > 1) {
		targetVendor = triple[1];
	} else {
		targetVendor = "pc";
	}

	if (triple.size() > 2) {
		targetSystem = triple[2];
	} else {
		targetSystem = "windows";
	}

	if (triple.size() > 3) {
		targetABI = triple[3];
	} else {
		targetABI = "unknown";
	}
}
