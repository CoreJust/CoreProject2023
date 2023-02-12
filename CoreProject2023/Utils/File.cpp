#include "File.h"
#include <fstream>
#include <filesystem>

std::string readFile(const std::string& file) {
	std::ifstream f(file);
	std::string tmp, result;
	while (std::getline(f, tmp)) {
		for (auto& ch : tmp) {
			if (ch == '\r') {
				ch = '\n';
			}
		}

		result.append(tmp) += '\n';
	}

	f.close();
	return result;
}

void createFileIfNotExists(const std::string& file) {
	if (!std::filesystem::exists(file)) {
		std::ofstream tmp(file, std::ofstream::binary | std::ofstream::trunc | std::ofstream::out);
		tmp.close();
	}
}
