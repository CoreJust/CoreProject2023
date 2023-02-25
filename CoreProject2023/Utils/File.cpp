#include "File.h"
#include <fstream>
#include <filesystem>
#include <iostream>

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
		std::ofstream tmp(file, std::ofstream::out);
		if (!tmp) {
			std::cout << "\aFailed to create: " << file << std::endl;
		}

		tmp.close();
	}
}

std::string concatPaths(const std::string& first, std::string second) {
	std::filesystem::path result = first;
	if (result.is_relative()) {
		result = std::filesystem::absolute(result);
	}

	while (second.size() >= 3 && second.substr(0, 3) == "../") {
		second = second.substr(3);
		result = result.parent_path();
	}

	result += "/";
	result += second;

	if (std::filesystem::is_directory(result)) {
		return result.string() + '/';
	}

	return result.string();
}
