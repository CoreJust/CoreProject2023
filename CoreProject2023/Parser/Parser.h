#pragma once
#include <memory>
#include "BasicParser.h"
#include "AST/Decls/Declaration.h"
#include "AST/States/Statement.h"
#include "AST/Exprs/Expression.h"

class Parser final : public BasicParser {
	u64 m_truePos = 0;

public:
	Parser(std::vector<Token>& tokens);

	std::vector<std::unique_ptr<Declaration>> parse();

private:
	std::unique_ptr<Declaration> declaration();
	void useDeclaration();
	std::unique_ptr<Declaration> structDeclaration();
	std::unique_ptr<Declaration> methodDeclaration(std::shared_ptr<TypeNode> parentType);
	std::unique_ptr<Declaration> fieldDeclaration(std::shared_ptr<TypeNode> parentType);
	std::unique_ptr<Declaration> functionDeclaration();
	std::unique_ptr<Declaration> variableDeclaration();

	std::unique_ptr<Statement> stateOrBlock();
	std::unique_ptr<Statement> statement();
	std::unique_ptr<Statement> variableDefStatement();
	std::unique_ptr<Statement> whileStatement();
	std::unique_ptr<Statement> ifElseStatement();

	std::unique_ptr<Expression> expression();
	std::unique_ptr<Expression> assignment();
	std::unique_ptr<Expression> logical();
	std::unique_ptr<Expression> conditional();
	std::unique_ptr<Expression> rangeAndAs();
	std::unique_ptr<Expression> bitwise();
	std::unique_ptr<Expression> additive();
	std::unique_ptr<Expression> multiplicative();
	std::unique_ptr<Expression> degree();
	std::unique_ptr<Expression> unary();
	std::unique_ptr<Expression> postfix();
	std::unique_ptr<Expression> primary();

	std::unique_ptr<Expression> parseFunctionValue(std::string moduleName, std::string name);

private:
	void functionCallError(
		std::string moduleName,
		std::string name,
		const std::vector<std::unique_ptr<Type>>& argTypes,
		bool isMultipleFunctionsFound // true if searched exact functions, false if tried to chooseS
	);

private:
	void skipAnnotation();
};