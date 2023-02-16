#pragma once
#include "Declaration.h"
#include "../States/Statement.h"
#include <Module/Symbols/Function.h>

class MethodDeclaration : public Declaration {
	FRIEND_CLASS_VISITORS

public:
	MethodDeclaration(Function* method, std::unique_ptr<Statement> body);

	void accept(Visitor* visitor, std::unique_ptr<Declaration>& node) override;
	void generate() override;

private:
	void generateConstructor();

private:
	Function* m_method;
	std::unique_ptr<Statement> m_body;
};

extern Function* g_function;