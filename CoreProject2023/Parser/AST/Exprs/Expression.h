#pragma once
#include "../INode.h"
#include <Module/Type.h>

class Expression : public INode {
protected:
	std::unique_ptr<Type> m_type;

public:
	virtual void accept(Visitor* visitor, std::unique_ptr<Expression>& node) = 0;
	virtual llvm::Value* generate() = 0;
	virtual llvm::Value* generateRValue() = 0;

	const std::unique_ptr<Type>& getType() const;
};