#pragma once
#include <memory>
#include <Parser/AST/AST.h>

class Visitor {
public:
	virtual void visit(ValueExpr* expr, std::unique_ptr<Expression>& node);
};