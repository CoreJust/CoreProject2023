#include "File.h"
#include <fstream>
#include <filesystem>

std::string readFile(const std::string& file) {
	std::ifstream f(file);
	std::string tmp, result;
	while (std::getline(f, tmp))
		result.append(tmp) += '\n';

	f.close();
	return result;
}

void createFileIfNotExists(const std::string& file) {
	if (!std::filesystem::exists(file)) {
		std::ofstream tmp(file);
		tmp.close();
	}
}
