#pragma once
#include "../INode.h"
#include <Module/Symbols/Type.h>

namespace llvm {
	class Value;
};

class Expression : public INode {
protected:
	std::shared_ptr<Type> m_type;

public:
	virtual void accept(Visitor* visitor, std::unique_ptr<Expression>& node) = 0;
	virtual llvm::Value* generate() = 0;

	const std::shared_ptr<Type>& getType() const;
	virtual bool isCompileTime() const;
	bool isLVal() const;
};