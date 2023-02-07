#pragma once
#include "Declaration.h"
#include "../States/Statement.h"
#include <Module/Function.h>

class FunctionDeclaration : public Declaration {
	FRIEND_CLASS_VISITORS

public:
	FunctionDeclaration(Function* func, std::unique_ptr<Statement> body);

	void accept(Visitor* visitor, std::unique_ptr<Declaration>& node) override;
	void generate() override;

private:
	Function* m_function;
	std::unique_ptr<Statement> m_body;
};

extern Function* g_function;