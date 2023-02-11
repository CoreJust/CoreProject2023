#include "Project.h"

Project::Project(const std::string& projectFile) {

}

Project::Project() {

}

std::string Project::getMainFilePath() {
	return "C:/Users/egor2/source/repos/CoreProject2023/examples/test.core";
}

std::vector<std::string> Project::getImportedPaths() {
	return std::vector<std::string>({ "C:/Users/egor2/source/repos/CoreProject2023/CoreStdLib/" });
}

std::string Project::getAdditionalLinkDirectoriesString() const {
	return " C:/Users/egor2/source/repos/CoreProject2023/CoreStdLib/built/crt.o";
}
