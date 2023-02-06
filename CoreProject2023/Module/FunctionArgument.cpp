#include "FunctionArgument.h"

Argument::Argument(std::string name, std::unique_ptr<Type> type)
	: name(std::move(name)), type(std::move(type)) {

}

Argument::Argument(Argument& other)
	: Argument(std::move(other)) {

}
