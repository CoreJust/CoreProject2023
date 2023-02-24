#include "FunctionArgument.h"

Argument::Argument(std::string name, std::shared_ptr<Type> type)
	: name(std::move(name)), type(std::move(type)) {

}

Argument::Argument(const Argument& other)
	: Argument(other.name, other.type) {

}
