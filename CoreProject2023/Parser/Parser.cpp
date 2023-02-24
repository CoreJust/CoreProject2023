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
		}

		break;
	}

	if (m_pos >= m_toks.size()) {
		return nullptr;
	}

	if (match(TokenType::STRUCT)) {
		return structDeclaration();
	} else if (match(TokenType::DEF)) {
		return functionDeclaration();
	} else {
		return variableDeclaration();
	}
}

void Parser::useDeclaration() {
	while (!match(TokenType::SEMICOLON)) {
		next();
	}
}

std::unique_ptr<Declaration> Parser::structDeclaration() {
	std::string name = consume(TokenType::WORD).data;
	std::shared_ptr<TypeNode> typeNode = g_module->getType(m_pos - 2);

	std::shared_ptr<TypeNode> previousGType = g_type;
	g_type = typeNode;

	consume(TokenType::LBRACE);

	std::vector<std::unique_ptr<Declaration>> fields;
	std::vector<std::unique_ptr<Declaration>> methods;
	while (!match(TokenType::RBRACE)) {
		while (match(TokenType::AT)) {
			skipAnnotation();
		}

		if (match(TokenType::DEF)) {
			methods.push_back(methodDeclaration(typeNode));
		} else {
			fields.push_back(fieldDeclaration(typeNode));
		}
	}

	g_type = previousGType;
	return std::make_unique<TypeDeclaration>(typeNode, std::move(fields), std::move(methods));
}

std::unique_ptr<Declaration> Parser::methodDeclaration(std::shared_ptr<TypeNode> parentType) {
	u64 tokenPos = m_pos;

	match(TokenType::PUBLIC);
	match(TokenType::PROTECTED);
	match(TokenType::PRIVATE);

	bool isStatic = match(TokenType::STATIC);
	match(TokenType::VIRTUAL);
	match(TokenType::ABSTRACT);

	match(TokenType::NATIVE);

	std::string alias;
	std::unique_ptr<Type> returnType;
	FunctionKind funcKind = FunctionKind::COMMON;
	if (match(TokenType::TYPE)) {
		funcKind = FunctionKind::CONSTRUCTOR;
		returnType = TypeParser(m_toks, m_pos).consumeType();
		alias = "type$" + returnType->toString();
	} else if (match(TokenType::THIS)) {
		funcKind = FunctionKind::CONSTRUCTOR;
		alias = "this";
		returnType = TypeNode::genType(parentType);
	} else if (match(TokenType::WORD)) {
		alias = peek(-1).data;
	} else { // operator-functions
		funcKind = FunctionKind::OPERATOR;
		alias = m_toks[m_pos].data;
		if (m_toks[m_pos].type == TokenType::LPAR || m_toks[m_pos].type == TokenType::LBRACKET) {
			m_pos++;
		}

		m_pos++;
	}

	// Arguments
	g_module->addBlock();
	consume(TokenType::LPAR);
	size_t i = 0;

	std::vector<std::unique_ptr<Type>> argTypes;

	if (!isStatic) {
		argTypes.push_back(std::make_unique<PointerType>(BasicType::XVAL_REFERENCE, TypeNode::genType(parentType))); // this
	}

	if (!match(TokenType::RPAR)) {
		do {
			if (!match(TokenType::ETCETERA)) {
				VariableQualities qualities;
				qualities.setVisibility(Visibility::LOCAL);
				
				std::unique_ptr<Type> argType = TypeParser(m_toks, m_pos).consumeType();

				if (argType->isConst) {
					qualities.setVariableType(VariableType::CONST);
				}

				std::string name = consume(TokenType::WORD).data;
				argTypes.push_back(argType->copy());
				g_module->addLocalVariable(name, std::move(argType), qualities, nullptr);
			}

			i++;
		} while (match(TokenType::COMMA));

		consume(TokenType::RPAR);
	}

	Function* function;
	if (funcKind == FunctionKind::COMMON || funcKind == FunctionKind::DESTRUCTOR) {
		function = parentType->getMethod(alias, argTypes, {}, isStatic);
	} else {
		function = g_module->getFunction(tokenPos);
	}

	ASSERT(function, "cannot be null");

	TypeParser(m_toks, m_pos).parseTypeOrGetNoType();
	if (function->prototype.getQualities().isNative()) {
		consume(TokenType::SEMICOLON);
		return std::make_unique<MethodDeclaration>(function, nullptr);
	} else {
		if (!isStatic) {
			if (function->prototype.getQualities().getFunctionKind() == FunctionKind::CONSTRUCTOR) {
				g_module->addLocalVariable("this", function->prototype.getReturnType()->copy(), VariableQualities(), nullptr);
			} else {
				g_module->addLocalVariable("this", argTypes[0]->copy(), VariableQualities(), nullptr);
			}
		}

		// Short function, like def a() i32 = 10;
		if (match(TokenType::EQ)) {
			std::unique_ptr<Expression> expr = expression();
			std::unique_ptr<Statement> body = std::make_unique<ReturnStatement>(std::move(expr));

			g_module->deleteBlock();
			consume(TokenType::SEMICOLON);
			return std::make_unique<MethodDeclaration>(function, std::move(body));
		} else if (match(TokenType::LBRACE)) {
			m_pos--;
			std::unique_ptr<Statement> body = stateOrBlock();

			g_module->deleteBlock();
			return std::make_unique<MethodDeclaration>(function, std::move(body));
		} else {
			ErrorManager::parserError(
				ErrorID::E2104_FUNCTION_BODY_MISMATCHED,
				getCurrLine(),
				"function: " + function->prototype.toString()
			);

			return nullptr;
		}
	}
}

std::unique_ptr<Declaration> Parser::fieldDeclaration(std::shared_ptr<TypeNode> parentType) {
	match(TokenType::PUBLIC);
	match(TokenType::PROTECTED);
	match(TokenType::PRIVATE);

	bool isStatic = match(TokenType::STATIC);

	TypeParser(m_toks, m_pos).skipConsumeType();

	std::string alias = consume(TokenType::WORD).data;
	Visibility visibility = (g_type && g_type->isEquals(parentType))
		? Visibility::PRIVATE : Visibility::PUBLIC;

	Variable* variable = parentType->getField(alias, visibility, isStatic);
	ASSERT(variable, "cannot be null");

	std::unique_ptr<Expression> expr = nullptr;
	if (match(TokenType::EQ)) {
		expr = expression();
	}

	consume(TokenType::SEMICOLON);
	return std::make_unique<FieldDeclaration>(parentType, variable, std::move(expr));
}

std::unique_ptr<Declaration> Parser::functionDeclaration() {
	Function* function = g_module->getFunction(m_pos);

	match(TokenType::NATIVE);
	if (match(TokenType::TYPE)) {
		TypeParser(m_toks, m_pos).skipConsumeType();
	} else if (!match(TokenType::WORD)) { // operator-functions and type conversions
		if (m_toks[m_pos].type == TokenType::LPAR || m_toks[m_pos].type == TokenType::LBRACKET) {
			m_pos++;
		}

		m_pos++;
	}

	// Arguments
	g_module->addBlock();
	consume(TokenType::LPAR);
	size_t i = 0;

	if (!match(TokenType::RPAR)) {
		do {
			if (!match(TokenType::ETCETERA))  {
				Argument& argument = function->prototype.args()[i];
				VariableQualities qualities;
				qualities.setVisibility(Visibility::LOCAL);

				TypeParser(m_toks, m_pos).skipConsumeType();

				if (argument.type->isConst) {
					qualities.setVariableType(VariableType::CONST);
				}

				std::string name = consume(TokenType::WORD).data;
				g_module->addLocalVariable(argument.name, argument.type->copy(), qualities, nullptr);
			}

			i++;
		} while (match(TokenType::COMMA));

		consume(TokenType::RPAR);
	}

	TypeParser(m_toks, m_pos).parseTypeOrGetNoType();
	if (function->prototype.getQualities().isNative()) {
		consume(TokenType::SEMICOLON);
		g_module->deleteBlock();
		return std::make_unique<FunctionDeclaration>(function, nullptr);
	} else {
		if (function->prototype.getQualities().getFunctionKind() == FunctionKind::CONSTRUCTOR) {
			g_module->addLocalVariable("this", function->prototype.getReturnType()->copy(), VariableQualities(), nullptr);
		}

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
				"function: " + function->prototype.toString()
			);

			g_module->deleteBlock();
			return nullptr;
		}
	}
}

std::unique_ptr<Declaration> Parser::variableDeclaration() {
	Variable* variable = g_module->getVariable(m_pos);

	match(TokenType::CONST);
	match(TokenType::EXTERN);
	TypeParser(m_toks, m_pos).skipConsumeType();

	std::string alias = consume(TokenType::WORD).data;

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
		|| TypeParser(m_toks, m_pos).isType(true)) {
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

	if (matchRange(TokenType::EQ, TokenType::RSHIFT_EQ)) {
		AssignmentExpr::AssignmentOp op = AssignmentExpr::AssignmentOp(+peek(-1).type - +TokenType::EQ);
		return std::make_unique<AssignmentExpr>(std::move(result), assignment(), op);
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
		} if (match(TokenType::LBRACKET)) {
			std::unique_ptr<Expression> index = expression();
			consume(TokenType::RBRACKET);

			return std::make_unique<ArrayElementAccessExpr>(std::move(expr), std::move(index));
		} if (match(TokenType::INCREMENT)) {
			expr = std::make_unique<UnaryExpr>(std::move(expr), UnaryExpr::POST_INC);
			continue;
		} if (match(TokenType::DECREMENT)) {
			expr = std::make_unique<UnaryExpr>(std::move(expr), UnaryExpr::POST_DEC);
			continue;
		} if (match(TokenType::DOT)) { // member access
			m_pos++;
			std::string memberName = peek(-1).data; // it can be WORD or any number
			const std::unique_ptr<Type>& thisType = Type::getTheVeryType(expr->getType());

			if (thisType->basicType == BasicType::TYPE_NODE) { // can be method
				std::shared_ptr<TypeNode> typeNode = ((TypeNodeType*)thisType.get())->node;
				Visibility visibility = (g_type && g_type->isEquals(typeNode)) 
					? Visibility::PRIVATE : Visibility::PUBLIC;

				SymbolType symType = typeNode->getSymbolType(memberName, visibility, false);
				if (symType == SymbolType::VARIABLE) {
					expr = std::make_unique<FieldAccessExpr>(std::move(expr), memberName);
					continue;
				} else if (symType == SymbolType::FUNCTION) {
					expr = parseMethodCall(typeNode, std::move(expr), memberName, false);
					continue;
				}
			}

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

	if (match(TokenType::THIS)) {
		if (Variable* thisVar = g_module->getVariable("this")) {
			return std::make_unique<VariableExpr>("", thisVar);
		} else {
			ErrorManager::parserError(
				ErrorID::E2106_THIS_USED_OUTSIDE_TYPE, 
				getCurrLine(),
				""
			);
		}
	}

	// Type conversion or array expression
	if (auto type = TypeParser(m_toks, m_pos).tryParseType()) {
		if (match(TokenType::LPAR)) { // type conversion/constructor
			std::vector<std::unique_ptr<Expression>> args;
			if (!match(TokenType::RPAR)) {
				do {
					args.push_back(expression());
				} while (match(TokenType::COMMA));
				consume(TokenType::RPAR);
			}
			return std::make_unique<TypeConversionExpr>(std::move(args), std::move(type));
		} else if (match(TokenType::LBRACE)) { // array expression (like u8 {...})
			if (type->basicType == BasicType::ARRAY) {
				return parseArrayValue(std::move(type->asArrayType()->elementType), type->asArrayType()->size);
			} else {
				return parseArrayValue(std::move(type), 0);
			}
		} else if (match(TokenType::DOT)) { // static members
			if (std::unique_ptr<Type>& containingType = Type::getTheVeryType(type);
				containingType->basicType == BasicType::TYPE_NODE) {
				std::shared_ptr<TypeNode> typeNode = ((TypeNodeType*)containingType.get())->node;
				Visibility visibility = (g_type && g_type->isEquals(typeNode))
					? Visibility::PRIVATE : Visibility::PUBLIC;

				std::string name = consume(TokenType::WORD).data;
				SymbolType symType = typeNode->getSymbolType(name, visibility, true);

				// Variable
				if (symType == SymbolType::VARIABLE) {
					Variable* variable = typeNode->getField(name, visibility, true);
					if (variable) {
						return std::make_unique<VariableExpr>(typeNode, variable);
					}
				} else if (symType == SymbolType::FUNCTION) { // Function
					return parseMethodCall(typeNode, nullptr, name, true);
				}
				
				// No such symbol
				ErrorManager::parserError(
					ErrorID::E2006_NO_SUCH_MEMBER,
					getCurrLine(),
					"identifier: " + typeNode->name + name
				);
			} else {
				ErrorManager::parserError(
					ErrorID::E2006_NO_SUCH_MEMBER,
					getCurrLine(),
					"non-user-defined types do not have static members"
				);
			}
		}
	}

	// Array expression (without type specified)
	if (match(TokenType::LBRACE)) {
		return parseArrayValue(nullptr, 0);
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

std::unique_ptr<Expression> Parser::parseArrayValue(std::unique_ptr<Type> type, u64 size) {
	std::vector<std::unique_ptr<Expression>> values;
	while (!match(TokenType::RBRACE)) {
		values.push_back(expression());
		
		if (match(TokenType::RBRACE)) {
			break;
		} else {
			consume(TokenType::COMMA);
		}
	}

	return std::make_unique<ArrayExpr>(std::move(type), size, std::move(values));
}

std::unique_ptr<Expression> Parser::parseMethodCall(
	std::shared_ptr<TypeNode> typeNode, 
	std::unique_ptr<Expression> expr, 
	std::string memberName, 
	bool isStatic
) {
	if (match(TokenType::LESS)) { // template
		std::vector<std::unique_ptr<Type>> argTypes;
		argTypes.push_back(std::make_unique<TypeNodeType>(typeNode));

		if (!match(TokenType::GREATER)) {
			do {
				argTypes.push_back(TypeParser(m_toks, m_pos).consumeType());
			} while (match(TokenType::COMMA));
			consume(TokenType::GREATER);
		}

		Function* func = typeNode->getMethod(memberName, argTypes, {}, isStatic);
		if (func == nullptr) {
			functionCallError(typeNode->name, memberName, argTypes, true);
			return nullptr;
		} else {
			return std::make_unique<FunctionExpr>(func);
		}
	} else if (match(TokenType::LPAR)) { // function call
		std::vector<std::unique_ptr<Expression>> args;
		std::vector<std::unique_ptr<Type>> argTypes;
		std::vector<bool> isCompileTime;

		if (!isStatic) {
			argTypes.push_back(std::make_unique<PointerType>(BasicType::XVAL_REFERENCE, std::make_unique<TypeNodeType>(typeNode)));
			isCompileTime.push_back(expr->isCompileTime());
			args.push_back(std::move(expr));
		}

		while (!match(TokenType::RPAR)) {
			args.push_back(expression());
			argTypes.push_back(args.back()->getType()->copy());
			isCompileTime.push_back(args.back()->isCompileTime());
			if (peek().type != TokenType::RPAR) {
				consume(TokenType::COMMA);
			}
		}

		Visibility visibility = (g_type && g_type->isEquals(typeNode))
			? Visibility::PRIVATE : Visibility::PUBLIC;

		Function* func = typeNode->chooseMethod(memberName, argTypes, isCompileTime, visibility, isStatic);

		if (func == nullptr) {
			functionCallError(typeNode->name, memberName, argTypes, false);
			return nullptr;
		}

		if (!isStatic) {
			return std::make_unique<MethodCallExpr>(func, std::move(args));
		} else {
			return std::make_unique<FunctionCallExpr>(std::make_unique<FunctionExpr>(func), std::move(args));
		}
	} else {
		Visibility visibility = (g_type && g_type->isEquals(typeNode))
			? Visibility::PRIVATE : Visibility::PUBLIC;

		Function* func = typeNode->getMethod(memberName, visibility, isStatic);
		if (func == nullptr) {
			functionCallError(typeNode->name, memberName, {}, true);
			return nullptr;
		} else {
			return std::make_unique<FunctionExpr>(func);
		}
	}
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
