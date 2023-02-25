#pragma once
#include <vector>
#include <Utils/Defs.h>

enum class Configuration : u8 {
	Release = 0,
	Debug
};

enum class OptimizationLevel : u8 {
	O0 = 0, // Optimization off
	O1,
	O2,
	O3
};

enum class CompilationMode : u8 {
	Program = 0, // Would be compiled into a single object file
	Library
};

// For each stage there can be
struct CompilerOutput {
	enum OutputStage : u8 {
		Lexer = 0,
		ASTBeforeOpt,
		ASTAfterOpt,
		IRBeforeOpt,
		IRAfterOpt,

		// These 2 are only available in file mode, which stated as default
		ObjectData,
		ExecutableData,

		StageCount
	};

	enum OutputMode : u8 {
		NoOut = 0,
		Console,
		File // in that case the file is stated in outputFiles
	};

	OutputMode outputModes[OutputStage::StageCount];
	std::string outputFiles[OutputStage::StageCount];

	CompilerOutput();

	void setOutput(OutputStage stage, OutputMode mode, std::string outputFile = "");
	OutputMode getOutputMode(OutputStage stage);
	std::string getOutputFile(OutputStage stage);
};

struct ProjectSettings {
	Configuration configuration;
	OptimizationLevel optLevel;
	CompilationMode compilationMode;
	CompilerOutput output;

	std::string targetArch;
	std::string targetVendor;
	std::string targetSystem;
	std::string targetABI;

	std::string projectName;
	std::vector<std::string> additionalImportDirs;
	std::vector<std::string> additionalLinkedObjectFiles;
	std::vector<std::string> compiledCoreModules; // not necessarily all of them, for a program the main file would suffice

	ProjectSettings();
};
