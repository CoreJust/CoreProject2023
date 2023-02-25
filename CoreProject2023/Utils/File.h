#pragma once
#include <string>

std::string readFile(const std::string& file);
void createFileIfNotExists(const std::string& file);

// Second path must be relative
std::string concatPaths(const std::string& first, std::string second);