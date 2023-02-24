#pragma once
#include <string>
#include "Type.h"

struct Argument {
	std::string name;
	std::shared_ptr<Type> type;

	Argument(std::string name, std::shared_ptr<Type> type);
	Argument(const Argument& other);
};