#pragma once
#include <memory>
#include "../INode.h"

class Declaration : public INode {
public:
	virtual void accept(Visitor* visitor, std::unique_ptr<Declaration>& node) = 0;
	virtual void generate() = 0;
};