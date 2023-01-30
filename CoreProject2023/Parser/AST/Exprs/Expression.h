#pragma once
#include <memory>
#include "../INode.h"

class Expression : public INode {
public:
	virtual void accept(Visitor* visitor, std::unique_ptr<Expression>& node) = 0;
	virtual llvm::Value* generate() = 0;
};