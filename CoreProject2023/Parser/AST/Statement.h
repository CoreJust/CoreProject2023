#pragma once
#include <memory>
#include "INode.h"

class Statement : public INode {
public:
	virtual void accept(Visitor* visitor, std::unique_ptr<Statement>& node) = 0;
	virtual void generate() = 0;
};