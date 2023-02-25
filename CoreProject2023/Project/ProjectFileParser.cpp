#include "ProjectFileParser.h"
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <json.hpp>
#include <Utils/ErrorManager.h>
#include <Utils/File.h>

using namespace nlohmann;

ProjectFileParser::ProjectFileParser(ProjectSettings& settings)
	: m_settings(settings) {

}

void ProjectFileParser::loadSettingsFromFile(const std::string& fileName) {
	m_file = fileName;

	try {
		std::ifstream file(fileName);
		json data = json::parse(file);
		file.close();

		loadSettings(&data);
	} catch (...) {
		ErrorManager::projectSettingsError(
			ErrorID::E5004_FAILED_TO_LOAD_SETTINGS,
			-1,
			"probably the project settings file has a wrong JSON format"
		);
	}

	if (m_settings.projectName == "") {
		ErrorManager::projectSettingsError(
			ErrorID::E5005_NECESSARY_SETTING_NOT_FOUND,
			-1,
			"project name (\"name\")"
		);
	} else if (m_settings.compiledCoreModules.size() == 0) {
		ErrorManager::projectSettingsError(
			ErrorID::E5005_NECESSARY_SETTING_NOT_FOUND,
			-1,
			"modules to be compiled (\"modules\")"
		);
	} else if (m_settings.compilationMode == CompilationMode::Program
		&& m_settings.output.getOutputMode(CompilerOutput::ExecutableData) != CompilerOutput::File) {
		ErrorManager::projectSettingsError(
			ErrorID::E5005_NECESSARY_SETTING_NOT_FOUND,
			-1,
			"the output executable file (\"output\": { \"executable-data\" })"
		);
	} else if (m_settings.compilationMode == CompilationMode::Program
		&& m_settings.output.getOutputMode(CompilerOutput::ObjectData) != CompilerOutput::File) {
		ErrorManager::projectSettingsError(
			ErrorID::E5005_NECESSARY_SETTING_NOT_FOUND,
			-1,
			"the output object file (\"output\": { \"object-data\" })"
		);
	}
}

void ProjectFileParser::loadSettings(void* jsonData) {
	const json& data = *(json*)jsonData;

	auto getJsonAs = [](const json& d, json::value_t valTy, std::string keyName) -> const json& {
		if (d.type() != valTy) {
			ErrorManager::projectSettingsError(
				ErrorID::E5002_WRONG_SETTING_TYPE,
				-1,
				keyName + " cannot be " + d.type_name()
			);
		}

		return d;
	};

	auto getJsonVariant = [](const json& d, const std::vector<std::string>& values, std::string keyName) -> u8 {
		std::string asStr = d.type() == json::value_t::string ? std::string(d) : d.dump();
		std::transform(asStr.begin(), asStr.end(), asStr.begin(), [](unsigned char c) { return std::tolower(c); });
		for (u8 i = 0; i < values.size(); i++) {
			if (asStr == values[i]) {
				return i;
			}
		}

		ErrorManager::projectSettingsError(
			ErrorID::E5003_WRONG_SETTING_VALUE,
			-1,
			d + " is impossible value for " + keyName
		);
	};

	for (auto& [key, value] : data.items()) {
		if (key == "name") {
			m_settings.projectName = getJsonAs(value, json::value_t::string, key);
		} else if (key == "modules") {
			for (auto& path : getJsonAs(value, json::value_t::array, key)) {
				m_settings.compiledCoreModules.push_back(getAbsolutePath(getJsonAs(path, json::value_t::string, "module path")));
			}
		} else if (key == "configuration") {
			const json& d = getJsonAs(value, json::value_t::string, key);
			m_settings.configuration = Configuration(getJsonVariant(d, { "release", "debug" }, key));
		} else if (key == "target-arch") {
			m_settings.targetArch = getJsonAs(value, json::value_t::string, key);
		} else if (key == "target-vendor") {
			m_settings.targetVendor = getJsonAs(value, json::value_t::string, key);
		} else if (key == "target-system") {
			m_settings.targetSystem = getJsonAs(value, json::value_t::string, key);
		} else if (key == "target-abi") {
			m_settings.targetABI = getJsonAs(value, json::value_t::string, key);
		} else if (key == "opt-level") {
			const json& d = getJsonAs(value, json::value_t::number_unsigned, key);
			m_settings.optLevel = OptimizationLevel(getJsonVariant(d, { "0", "1", "2", "3" }, key));
		} else if (key == "compilation-mode") {
			const json& d = getJsonAs(value, json::value_t::string, key);
			m_settings.compilationMode = CompilationMode(getJsonVariant(d, { "program", "library" }, key));
		} else if (key == "output") {
			for (auto& [ stageStr, val ] : getJsonAs(value, json::value_t::object, key).items()) {
				const json& d = getJsonAs(val, json::value_t::array, stageStr);
				CompilerOutput::OutputStage stage = CompilerOutput::OutputStage(
					getJsonVariant(
						stageStr,
						{ "tokens", "ast", "optimized-ast", "llvm-ir", "optimized-llvm-ir", "object-data", "executable-data" },
						key
					)
				);

				CompilerOutput::OutputMode mode = CompilerOutput::OutputMode(getJsonVariant(
					d[0],
					{ "no-output", "console", "file" },
					key
				));

				std::string fileName;
				if (mode == CompilerOutput::File) {
					fileName = getAbsolutePath(getJsonAs(d[1], json::value_t::string, "output file"));
				}

				m_settings.output.setOutput(stage, mode, fileName);
			}
		} else if (key == "import-paths") {
			for (auto& path : getJsonAs(value, json::value_t::array, key)) {
				m_settings.additionalImportDirs.push_back(getAbsolutePath(getJsonAs(path, json::value_t::string, "import path")));
			}
		} else if (key == "additional-linked-files") {
			for (auto& path : getJsonAs(value, json::value_t::array, key)) {
				m_settings.additionalLinkedObjectFiles.push_back(getAbsolutePath(
					getJsonAs(path, json::value_t::string, "additional linked file path")
				));
			}
		} else {
			ErrorManager::projectSettingsError(
				ErrorID::E5001_UNKNOWN_SETTING,
				-1,
				key + " - no such setting"
			);
		}
	}
}

std::string ProjectFileParser::getAbsolutePath(const std::string& pathFromProjectFile) {
	std::filesystem::path path = pathFromProjectFile;
	if (path.is_absolute()) {
		return pathFromProjectFile;
	}

	std::filesystem::path projectFilePath = m_file;
	projectFilePath = projectFilePath.parent_path();
	return concatPaths(projectFilePath.string(), pathFromProjectFile);
}
