#include "Parser.h"
#include <Utils/ErrorManager.h>
#include "AST/AST.h"
#include <Module/Module.h>
#include "TypeParser.h"

static Token _NO_TOK = Token();

// used in INode.cpp
u64* g_pos;
std::vector<Token>* g_toks;


Parser::Parser(std::vector<Token>& tokens)
	: BasicParser(tokens, m_truePos) {
	g_pos = &m_pos;
	g_toks = &m_toks;
}

std::vector<std::unique_ptr<Declaration>> Parser::parse() {
	std::vector<std::unique_ptr<Declaration>> result;
	while (m_pos < m_toks.size()) {
		if (auto decl = declaration(); decl != nullptr) {
			result.push_back(std::move(decl));
		}
	}

	return result;
}

std::unique_ptr<Declaration> Parser::declaration() {
	while (true) {
		if (match(TokenType::AT)) {
			skipAnnotation();
			continue;
		} if (match(TokenType::TYPE)) {
			while (!match(TokenType::SEMICOLON)) {
				m_pos++;
			}

			continue;
		} if (match(TokenType::USE)) {
			useDeclaration();
			continue;
		} if (match(TokenType::STRUCT)) {
			structDeclaration();
			continue;
		}

		break;
	}

	if (m_pos >= m_toks.size()) {
		return nullptr;
	}

	if (match(TokenType::DEF)) {
		return functionDeclaration();
	} else {
		return variableDeclaration();
	}
}

void Parser::useDeclaration() {
	std::string moduleName = "";
	std::string name = consume(TokenType::WORD).data;
	SymbolType symType = g_module->getSymbolType(name);

	if (symType == SymbolType::MODULE && match(TokenType::DOT)) {
		moduleName = std::move(name);
		name = consume(TokenType::WORD).data;
		symType = g_module->getSymbolType(moduleName, name);
	}

	if (symType == SymbolType::NO_SYMBOL) {
		ErrorManager::parserError(
			ErrorID::E2003_UNKNOWN_IDENTIFIER,
			getCurrLine(),
			"identifier: " + (moduleName.size() ? moduleName + "." + name : name)
		);

		return;
	}

	std::string alias = "";
	if (match(TokenType::AS)) {
		alias = consume(TokenType::WORD).data;
	}
		
	g_module->addAlias(symType, moduleName, name, alias);

	consume(TokenType::SEMICOLON);
}

std::unique_ptr<Declaration> Parser::structDeclaration() {
	std::string name = consume(TokenType::WORD).data;
	TypeNode* typeNode = g_module->getType(name).get();

	// TODO: implement

	consume(TokenType::LBRACE);
	while (!match(TokenType::RBRACE)) {
		m_pos++;
	}

	return std::unique_ptr<Declaration>();
}

std::unique_ptr<Declaration> Parser::functionDeclaration() {
	// read function declaration
	std::string alias = "";

	match(TokenType::NATIVE);
	if (match(TokenType::WORD)) { // common function
		alias = m_toks[m_pos - 1].data;
	} else { // operator-functions and type conversions
		alias = m_toks[m_pos].data;
		if (m_toks[m_pos].type == TokenType::LPAR || m_toks[m_pos].type == TokenType::LBRACKET) {
			m_pos++;
		}

		m_pos++;
	}

	// Arguments
	std::vector<std::unique_ptr<Type>> argTypes;
	g_module->addBlock();
	consume(TokenType::LPAR);

	if (!match(TokenType::RPAR)) {
		do {
			if (!match(TokenType::ETCETERA))  {
				VariableQualities qualities;
				std::unique_ptr<Type> type = TypeParser(m_toks, m_pos).consumeType();

				if (type->isConst) {
					qualities.setVariableType(VariableType::CONST);
				}

				argTypes.push_back(type->copy());
				std::string name = consume(TokenType::WORD).data;
				g_module->addLocalVariable(name, std::move(type), qualities, nullptr);
			}
		} while (match(TokenType::COMMA));

		consume(TokenType::RPAR);
	}

	Function* function = g_module->getFunction("", alias, argTypes, { });

	TypeParser(m_toks, m_pos).parseTypeOrGetNoType();
	if (function->prototype.getQualities().isNative()) {
		consume(TokenType::SEMICOLON);
		return std::make_unique<FunctionDeclaration>(function, nullptr);
	} else {
		// Short function, like def a() i32 = 10;
		if (match(TokenType::EQ)) {
			std::unique_ptr<Expression> expr = expression();
			std::unique_ptr<Statement> body = std::make_unique<ReturnStatement>(std::move(expr));

			g_module->deleteBlock();
			consume(TokenType::SEMICOLON);
			return std::make_unique<FunctionDeclaration>(function, std::move(body));
		} else if (match(TokenType::LBRACE)) {
			m_pos--;
			std::unique_ptr<Statement> body = stateOrBlock();

			g_module->deleteBlock();
			return std::make_unique<FunctionDeclaration>(function, std::move(body));
		} else {
			ErrorManager::parserError(
				ErrorID::E2104_FUNCTION_BODY_MISMATCHED,
				getCurrLine(), 
				"function: " + alias
			);

			return nullptr;
		}
	}
}

std::unique_ptr<Declaration> Parser::variableDeclaration() {
	match(TokenType::CONST);
	match(TokenType::EXTERN);
	TypeParser(m_toks, m_pos).consumeType();

	std::string alias = consume(TokenType::WORD).data;
	Variable* variable = g_module->getVariable(alias);

	std::unique_ptr<Expression> expr = nullptr;
	if (match(TokenType::EQ)) {
		expr = expression();
	}
	
	consume(TokenType::SEMICOLON);
	return std::make_unique<VariableDeclaration>(variable, std::move(expr));
}

std::unique_ptr<Statement> Parser::stateOrBlock() {
	if (match(TokenType::LBRACE)) {
		g_module->addBlock();
		std::vector<std::unique_ptr<Statement>> states;
		while (!match(TokenType::RBRACE)) {
			states.push_back(statement());
		}

		g_module->deleteBlock();
		return std::make_unique<BlockStatement>(std::move(states));
	} else {
		return statement();
	}
}

std::unique_ptr<Statement> Parser::statement() {
	if (match(TokenType::IF)) {
		return ifElseStatement();
	} else if(match(TokenType::WHILE)) {
		return whileStatement();
	} else if (peek().type == TokenType::LBRACE) {
		return stateOrBlock();
	} else if (peek().type == TokenType::VAR || peek().type == TokenType::CONST
		|| TypeParser(m_toks, m_pos).isType()) {
		return variableDefStatement();
	}

	std::unique_ptr<Statement> result;
	if (match(TokenType::RETURN)) {
		if (match(TokenType::SEMICOLON)) {
			return std::make_unique<ReturnStatement>(nullptr);
		} else {
			result = std::make_unique<ReturnStatement>(expression());
		}
	} else {
		result = std::make_unique<ExpressionStatement>(expression());
	}

	consume(TokenType::SEMICOLON);
	return result;
}

std::unique_ptr<Statement> Parser::variableDefStatement() {
	std::unique_ptr<Type> type;
	bool isConst = match(TokenType::CONST);
	if (!match(TokenType::VAR)) {
		type = TypeParser(m_toks, m_pos).consumeType();
		type->isConst = isConst;
	}

	std::string alias = consume(TokenType::WORD).data;

	std::unique_ptr<Expression> expr = nullptr;
	if (match(TokenType::EQ)) {
		expr = expression();
	}

	if (!type) {
		if (!expr) {
			ErrorManager::parserError(
				ErrorID::E2101_VAR_HAS_NO_INIT,
				getCurrLine(),
				"variable: " + alias
			);
		} else {
			type = expr->getType()->copy();
		}
	}

	VariableQualities qualities;
	qualities.setVariableType(isConst ? VariableType::CONST : VariableType::COMMON);
	g_module->addLocalVariable(alias, type->copy(), qualities, nullptr);
	Variable variable(alias, std::move(type), qualities, nullptr);

	consume(TokenType::SEMICOLON);
	return std::make_unique<VariableDefStatement>(std::move(variable), std::move(expr));
}

std::unique_ptr<Statement> Parser::whileStatement() {
	std::unique_ptr<Expression> condition;
	std::unique_ptr<Statement> body;
	bool hasParen = match(TokenType::LPAR);

	g_module->addBlock();
	condition = expression();
	if (hasParen)
		consume(TokenType::RPAR);

	body = stateOrBlock();

	g_module->deleteBlock();
	return std::make_unique<WhileStatement>(std::move(condition), std::move(body));
}

std::unique_ptr<Statement> Parser::ifElseStatement() {
	std::vector<std::unique_ptr<Expression>> conditions;
	std::vector<std::unique_ptr<Statement>> bodies;

	do {
		if (conditions.size() && peek().type == TokenType::ELSE) {
			m_pos += 2;
		}

		bool hasParen = match(TokenType::LPAR);
		conditions.push_back(expression());
		if (hasParen)
			consume(TokenType::RPAR);

		g_module->addBlock();
		bodies.push_back(stateOrBlock());
		g_module->deleteBlock();
	} while (match(TokenType::ELIF) || (peek().type == TokenType::ELSE && peek(1).type == TokenType::IF));

	if (match(TokenType::ELSE)) {
		g_module->addBlock();
		bodies.push_back(stateOrBlock());
		g_module->deleteBlock();
	}

	return std::make_unique<IfElseStatement>(std::move(bodies), std::move(conditions));
}

std::unique_ptr<Expression> Parser::expression() {
	return assignment();
}

std::unique_ptr<Expression> Parser::assignment() {
	std::unique_ptr<Expression> result = logical();

	if (match(TokenType::EQ)) {
		return std::make_unique<AssignmentExpr>(std::move(result), assignment());
	}

	return result;
}

std::unique_ptr<Expression> Parser::logical() {
	std::unique_ptr<Expression> result = conditional();

	while (true) {
		if (match(TokenType::ANDAND)) {
			result = std::make_unique<BinaryExpr>(std::move(result), conditional(), BinaryExpr::LOGICAL_AND);
			continue;
		} if (match(TokenType::OROR)) {
			result = std::make_unique<BinaryExpr>(std::move(result), conditional(), BinaryExpr::LOGICAL_OR);
			continue;
		}

		break;
	}

	return result;
}

std::unique_ptr<Expression> Parser::conditional() {
	std::unique_ptr<Expression> initialExpr = rangeAndAs();
	std::vector<std::unique_ptr<Expression>> exprs;
	std::vector<ConditionalExpr::ConditionOp> ops;

	auto addCondition = [&](ConditionalExpr::ConditionOp op) {
		if (exprs.size() == 0) {
			exprs.push_back(std::move(initialExpr));
		}

		exprs.push_back(rangeAndAs());
		ops.push_back(op);
	};

	while (true) {
		if (match(TokenType::EQEQ)) {
			addCondition(ConditionalExpr::EQUALS);
			continue;
		} if (match(TokenType::EXCLEQ)) {
			addCondition(ConditionalExpr::NOT_EQUALS);
			continue;
		} if (match(TokenType::LESS)) {
			addCondition(ConditionalExpr::LESS);
			continue;
		} if (match(TokenType::LESSEQ)) {
			addCondition(ConditionalExpr::LESS_OR_EQUAL);
			continue;
		} if (match(TokenType::GREATER)) {
			addCondition(ConditionalExpr::GREATER);
			continue;
		} if (match(TokenType::GREATEREQ)) {
			addCondition(ConditionalExpr::GREATER_OR_EQUAL);
			continue;
		}

		break;
	}
	
	if (exprs.size() == 0) {
		return initialExpr;
	} else {
		return std::make_unique<ConditionalExpr>(std::move(exprs), std::move(ops));
	}
}

std::unique_ptr<Expression> Parser::rangeAndAs() {
	// TODO: implement
	return bitwise();
}

std::unique_ptr<Expression> Parser::bitwise() {
	std::unique_ptr<Expression> result = additive();

	while (true) {
		if (match(TokenType::AND)) {
			result = std::make_unique<BinaryExpr>(std::move(result), additive(), BinaryExpr::AND);
			continue;
		} if (match(TokenType::OR)) {
			result = std::make_unique<BinaryExpr>(std::move(result), additive(), BinaryExpr::OR);
			continue;
		} if (match(TokenType::XOR)) {
			result = std::make_unique<BinaryExpr>(std::move(result), additive(), BinaryExpr::XOR);
			continue;
		} if (match(TokenType::LSHIFT)) {
			result = std::make_unique<BinaryExpr>(std::move(result), additive(), BinaryExpr::LSHIFT);
			continue;
		} if (match(TokenType::RSHIFT)) {
			result = std::make_unique<BinaryExpr>(std::move(result), additive(), BinaryExpr::RSHIFT);
			continue;
		}

		break;
	}

	return result;
}

std::unique_ptr<Expression> Parser::additive() {
	std::unique_ptr<Expression> result = multiplicative();

	while (true) {
		if (match(TokenType::PLUS)) {
			result = std::make_unique<BinaryExpr>(std::move(result), multiplicative(), BinaryExpr::PLUS);
			continue;
		} if (match(TokenType::MINUS)) {
			result = std::make_unique<BinaryExpr>(std::move(result), multiplicative(), BinaryExpr::MINUS);
			continue;
		}

		break;
	}

	return result;
}

std::unique_ptr<Expression> Parser::multiplicative() {
	std::unique_ptr<Expression> result = degree();

	while (true) {
		if (match(TokenType::STAR)) {
			result = std::make_unique<BinaryExpr>(std::move(result), degree(), BinaryExpr::MULT);
			continue;
		} if (match(TokenType::SLASH)) {
			result = std::make_unique<BinaryExpr>(std::move(result), degree(), BinaryExpr::DIV);
			continue;
		} if (match(TokenType::DSLASH)) {
			result = std::make_unique<BinaryExpr>(std::move(result), degree(), BinaryExpr::IDIV);
			continue;
		} if (match(TokenType::PERCENT)) {
			result = std::make_unique<BinaryExpr>(std::move(result), degree(), BinaryExpr::MOD);
			continue;
		}

		break;
	}

	return result;
}

std::unique_ptr<Expression> Parser::degree() {
	std::unique_ptr<Expression> result = unary();

	while (true) {
		if (match(TokenType::POWER)) {
			result = std::make_unique<BinaryExpr>(std::move(result), unary(), BinaryExpr::POWER);
			continue;
		}

		break;
	}

	return result;
}

std::unique_ptr<Expression> Parser::unary() {
	while (true) {
		if (match(TokenType::PLUS)) {
			return std::make_unique<UnaryExpr>(unary(), UnaryExpr::PLUS);
		} if (match(TokenType::MINUS)) {
			return std::make_unique<UnaryExpr>(unary(), UnaryExpr::MINUS);
		}if (match(TokenType::INCREMENT)) {
			return std::make_unique<UnaryExpr>(unary(), UnaryExpr::PRE_INC);
		} if (match(TokenType::DECREMENT)) {
			return std::make_unique<UnaryExpr>(unary(), UnaryExpr::PRE_DEC);
		} if (match(TokenType::EXCL)) {
			return std::make_unique<UnaryExpr>(unary(), UnaryExpr::LOGICAL_NOT);
		} if (match(TokenType::TILDE)) {
			return std::make_unique<UnaryExpr>(unary(), UnaryExpr::NOT);
		} if (match(TokenType::AND)) {
			return std::make_unique<UnaryExpr>(unary(), UnaryExpr::ADRESS);
		} if (match(TokenType::STAR)) {
			return std::make_unique<UnaryExpr>(unary(), UnaryExpr::DEREF);
		} if (match(TokenType::REF)) {
			if (match(TokenType::CONST)) {
				return std::make_unique<UnaryExpr>(unary(), UnaryExpr::REF_CONST);
			} else {
				return std::make_unique<UnaryExpr>(unary(), UnaryExpr::REF);
			}
		} if (match(TokenType::MOVE)) {
			return std::make_unique<UnaryExpr>(unary(), UnaryExpr::MOVE);
		}

		break;
	}

	return postfix();
}

std::unique_ptr<Expression> Parser::postfix() {
	std::unique_ptr<Expression> expr = primary();
	while (true) {
		if (match(TokenType::LPAR)) {
			std::vector<std::unique_ptr<Expression>> args;
			while (!match(TokenType::RPAR)) {
				args.push_back(expression());
				if (peek().type != TokenType::RPAR) {
					consume(TokenType::COMMA);
				}
			}

			expr = std::make_unique<FunctionCallExpr>(std::move(expr), std::move(args));
			continue;
		} if (match(TokenType::INCREMENT)) {
			expr = std::make_unique<UnaryExpr>(std::move(expr), UnaryExpr::POST_INC);
			continue;
		} if (match(TokenType::DECREMENT)) {
			expr = std::make_unique<UnaryExpr>(std::move(expr), UnaryExpr::POST_DEC);
			continue;
		} if (match(TokenType::DOT)) { // member access
			// TODO: add consideration of methods
			m_pos++;
			std::string memberName = peek(-1).data; // it can be WORD or any number
			expr = std::make_unique<FieldAccessExpr>(std::move(expr), memberName);
			continue;
		}

		break;
	}

	return expr;
}

std::unique_ptr<Expression> Parser::primary() {
	if (match(TokenType::LPAR)) {
		std::unique_ptr<Expression> result = expression();
		consume(TokenType::RPAR);
		return result;
	}

	// Literals
	if (match(TokenType::NUMBERI8)) {
		return std::make_unique<ValueExpr>(Value(BasicType::I8, _ValueUnion((i64)std::stol(peek(-1).data))));
	} if (match(TokenType::NUMBERI16)) {
		return std::make_unique<ValueExpr>(Value(BasicType::I16, _ValueUnion((i64)std::stol(peek(-1).data))));
	} if (match(TokenType::NUMBERI32)) {
		return std::make_unique<ValueExpr>(Value(BasicType::I32, _ValueUnion((i64)std::stol(peek(-1).data))));
	} if (match(TokenType::NUMBERI64)) {
		return std::make_unique<ValueExpr>(Value(BasicType::I64, _ValueUnion((i64)std::stol(peek(-1).data))));
	} if (match(TokenType::NUMBERU8)) {
		return std::make_unique<ValueExpr>(Value(BasicType::U8, _ValueUnion((u64)std::stoull(peek(-1).data))));
	} if (match(TokenType::NUMBERU16)) {
		return std::make_unique<ValueExpr>(Value(BasicType::U16, _ValueUnion((u64)std::stoull(peek(-1).data))));
	} if (match(TokenType::NUMBERU32)) {
		return std::make_unique<ValueExpr>(Value(BasicType::U32, _ValueUnion((u64)std::stoull(peek(-1).data))));
	} if (match(TokenType::NUMBERU64)) {
		return std::make_unique<ValueExpr>(Value(BasicType::U64, _ValueUnion((u64)std::stoull(peek(-1).data))));
	} if (match(TokenType::NUMBERF32)) {
		return std::make_unique<ValueExpr>(Value(BasicType::F32, _ValueUnion((f64)std::stod(peek(-1).data))));
	} if (match(TokenType::NUMBERF64)) {
		return std::make_unique<ValueExpr>(Value(BasicType::F64, _ValueUnion((f64)std::stod(peek(-1).data))));
	} if (match(TokenType::FALSE)) {
		return std::make_unique<ValueExpr>(Value(BasicType::BOOL, _ValueUnion((u64)0)));
	} if (match(TokenType::TRUE)) {
		return std::make_unique<ValueExpr>(Value(BasicType::BOOL, _ValueUnion((u64)1)));
	} if (match(TokenType::LETTER8)) {
		return std::make_unique<ValueExpr>(Value(BasicType::C8, _ValueUnion((i64)peek(-1).data[0])));
	} if (match(TokenType::LETTER16)) {
		return std::make_unique<ValueExpr>(Value(BasicType::C16, _ValueUnion((i64)*(i16*)&peek(-1).data[0])));
	} if (match(TokenType::LETTER32)) {
		return std::make_unique<ValueExpr>(Value(BasicType::C32, _ValueUnion((i64)*(i32*)&peek(-1).data[0])));
	} if (match(TokenType::TEXT8)) {
		return std::make_unique<ValueExpr>(Value(BasicType::STR8, _ValueUnion(peek(-1).data)));
	} if (match(TokenType::TEXT16)) {
		return std::make_unique<ValueExpr>(Value(BasicType::STR16, _ValueUnion(peek(-1).data)));
	} if (match(TokenType::TEXT32)) {
		return std::make_unique<ValueExpr>(Value(BasicType::STR32, _ValueUnion(peek(-1).data)));
	} if (match(TokenType::NULLPTR)) {
		return std::make_unique<ValueExpr>(Value(BasicType::POINTER, _ValueUnion()));
	}

	// Type conversion or array expression
	if (auto type = TypeParser(m_toks, m_pos).tryParseType()) {
		if (match(TokenType::LPAR)) { // type conversion/constructor
			std::unique_ptr<Expression> expr = expression();
			consume(TokenType::RPAR);
			return std::make_unique<TypeConversionExpr>(std::move(expr), std::move(type));
		} else if (match(TokenType::LBRACE)) { // array expression (like u8 {...})
			// TODO: implement
			consume(TokenType::RBRACE);
		}
	}
	
	// Identifier
	if (match(TokenType::WORD)) {
		std::string name = peek(-1).data;
		std::string moduleName = "";
		SymbolType symType = g_module->getSymbolType(name);

		// Module
		if (symType == SymbolType::MODULE) {
			consume(TokenType::DOT);
			moduleName = std::move(name);
			name = consume(TokenType::WORD).data;
			symType = g_module->getSymbolType(moduleName, name);
		}

		// Variable
		if (symType == SymbolType::VARIABLE) {
			Variable* variable = g_module->getVariable(moduleName, name);
			return std::make_unique<VariableExpr>(std::move(moduleName), variable);
		} else if (symType == SymbolType::FUNCTION) { // Function
			return parseFunctionValue(std::move(moduleName), std::move(name));
		} else if (symType == SymbolType::NO_SYMBOL) { // No such symbol
			ErrorManager::parserError(
				ErrorID::E2003_UNKNOWN_IDENTIFIER,
				getCurrLine(),
				"identifier: " + (moduleName.size() ? moduleName + "." + name : name)
			);
		}
	}

	ErrorManager::parserError(
		ErrorID::E2001_EXPRESSION_NOT_FOUND, 
		getCurrLine(),
		"No expression found"
	);

	return nullptr;
}

std::unique_ptr<Expression> Parser::parseFunctionValue(std::string moduleName, std::string name) {
	if (match(TokenType::LESS)) { // template
		std::vector<std::unique_ptr<Type>> argTypes;
		if (!match(TokenType::GREATER)) {
			do {
				argTypes.push_back(TypeParser(m_toks, m_pos).consumeType());
			} while (match(TokenType::COMMA));
			consume(TokenType::GREATER);
		}

		Function* func = g_module->getFunction(moduleName, name, argTypes, {});
		if (func == nullptr) {
			functionCallError(moduleName, name, argTypes, true);
			return nullptr;
		} else {
			return std::make_unique<FunctionExpr>(func);
		}
	} else if (match(TokenType::LPAR)) { // function call
		std::vector<std::unique_ptr<Expression>> args;
		std::vector<std::unique_ptr<Type>> argTypes;
		std::vector<bool> isCompileTime;

		while (!match(TokenType::RPAR)) {
			args.push_back(expression());
			argTypes.push_back(args.back()->getType()->copy());
			isCompileTime.push_back(args.back()->isCompileTime());
			if (peek().type != TokenType::RPAR) {
				consume(TokenType::COMMA);
			}
		}

		Function* func = g_module->chooseFunction(moduleName, name, argTypes, isCompileTime);
		if (func == nullptr) {
			functionCallError(moduleName, name, argTypes, false);
			return nullptr;
		}

		std::unique_ptr<Expression> functionExpr = std::make_unique<FunctionExpr>(func);
		return std::make_unique<FunctionCallExpr>(std::move(functionExpr), std::move(args));
	} else {
		Function* func = g_module->getFunction(moduleName, name);
		if (func == nullptr) {
			functionCallError(moduleName, name, {}, true);
			return nullptr;
		} else {
			return std::make_unique<FunctionExpr>(func);
		}
	}
}

void Parser::functionCallError(
	std::string moduleName, 
	std::string name, 
	const std::vector<std::unique_ptr<Type>>& argTypes, 
	bool isMultipleFunctionsFound
) {
	std::string error = "function: ";
	if (moduleName.size()) {
		error += moduleName;
		error += '.';
	}

	error += name;
	error += isMultipleFunctionsFound ? '<' : '(';
	for (auto& type : argTypes) {
		error += type->toString();
		error += ", ";
	}

	error.pop_back();
	error.back() = isMultipleFunctionsFound ? '>' : ')';

	ErrorID errorID = isMultipleFunctionsFound ?
		ErrorID::E2004_MANY_FUNCTIONS_WITH_SUCH_NAME
		: ErrorID::E2005_NO_SUITABLE_FUNCTION;

	ErrorManager::parserError(
		errorID,
		getCurrLine(),
		error
	);
}

void Parser::skipAnnotation() {
	m_pos++;
}
