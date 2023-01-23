#include "File.h"
#include <fstream>

std::string readFile(const std::string& file) {
	std::ifstream f(file);
	std::string tmp, result;
	while (std::getline(f, tmp))
		result.append(tmp) += '\n';

	f.close();
	return result;
}