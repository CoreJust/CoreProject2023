#pragma once
#include <string>
#include "Type.h"

struct Argument {
	std::string name;
	std::unique_ptr<Type> type;

	Argument(std::string name, std::unique_ptr<Type> type);
	Argument(Argument&&) = default;
	Argument(Argument& other);
};