#include "Parser.h"
#include <Utils/ErrorManager.h>
#include "AST/AST.h"
#include <Module/Module.h>
#include "TypeParser.h"

static Token _NO_TOK = Token();

Parser::Parser(std::vector<Token> tokens) : m_toks(std::move(tokens)), m_pos(0) { }

std::vector<std::unique_ptr<Declaration>> Parser::parse() {
	std::vector<std::unique_ptr<Declaration>> result;
	while (m_pos < m_toks.size()) {
		result.push_back(declaration());
	}

	return result;
}

std::unique_ptr<Declaration> Parser::declaration() {
	if (match(TokenType::DEF)) {
		return functionDeclaration();
	} else {
		return variableDeclaration();
	}
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

	Function* function = g_module->getFunction(alias);

	// Arguments
	consume(TokenType::LPAR);
	if (!match(TokenType::RPAR)) {
		do {
			match(TokenType::CONST);
			TypeParser(m_toks, m_pos).consumeType();
			consume(TokenType::WORD);
		} while (match(TokenType::COMMA));

		consume(TokenType::RPAR);
	}

	TypeParser(m_toks, m_pos).parseTypeOrGetNoType();
	if (function->qualities.isNative()) {
		consume(TokenType::SEMICOLON);
		return std::make_unique<FunctionDeclaration>(function, nullptr);
	} else {
		// Short function, like def a() i32 = 10;
		if (match(TokenType::EQ)) {
			std::unique_ptr<Expression> expr = expression();
			std::unique_ptr<Statement> body = std::make_unique<ReturnStatement>(std::move(expr));
			return std::make_unique<FunctionDeclaration>(function, std::move(body));
		} else if (match(TokenType::LBRACE)) {
			m_pos--;
			std::unique_ptr<Statement> body = stateOrBlock();
			return std::make_unique<FunctionDeclaration>(function, std::move(body));
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
		std::vector<std::unique_ptr<Statement>> states;
		while (!match(TokenType::RBRACE)) {
			states.push_back(statement());
		}

		return std::make_unique<BlockStatement>(std::move(states));
	} else {
		return statement();
	}
}

std::unique_ptr<Statement> Parser::statement() {
	std::unique_ptr<Statement> result;
	if (match(TokenType::RETURN)) {
		result = std::make_unique<ReturnStatement>(expression());
	} else {
		result = std::make_unique<ExpressionStatement>(expression());
	}

	consume(TokenType::SEMICOLON);
	return result;
}

std::unique_ptr<Expression> Parser::expression() {
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
		}

		break;
	}

	return expr;
}

std::unique_ptr<Expression> Parser::primary() {
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
	if (auto type = TypeParser(m_toks, m_pos).parseType()) {
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
		if (symType == SymbolType::MODULE) {
			consume(TokenType::DOT);
			moduleName = std::move(name);
			name = consume(TokenType::WORD).data;
			symType = g_module->getSymbolType(moduleName, name);
		}

		if (symType == SymbolType::VARIABLE) {
			Variable* variable = g_module->getVariable(moduleName, name);
			return std::make_unique<VariableExpr>(variable);
		} else if (symType == SymbolType::FUNCTION) {
			Function* function = g_module->getFunction(moduleName, name);
			return std::make_unique<FunctionExpr>(function);
		}
	}

	ErrorManager::parserError(ErrorID::E2001_EXPRESSION_NOT_FOUND, getCurrLine(), "No expression found");
	return nullptr;
}

Token& Parser::consume(TokenType type) {
	if (!match(type))
		ErrorManager::parserError(ErrorID::E2002_UNEXPECTED_TOKEN, getCurrLine(), "expected " + Token::toString(type));

	return peek(-1);
}

bool Parser::match(TokenType type) {
	if (peek().type != type)
		return false;

	m_pos++;
	return true;
}

bool Parser::matchRange(TokenType from, TokenType to) {
	auto type = peek().type;
	if (type < from || type > to)
		return false;

	m_pos++;
	return true;
}

Token& Parser::next() {
	if (m_pos >= m_toks.size())
		return _NO_TOK;

	return m_toks[m_pos++];
}

Token& Parser::peek(int rel) {
	u64 pos = m_pos + rel;
	if (pos >= m_toks.size())
		return _NO_TOK;

	return m_toks[pos];
}

int Parser::getCurrLine() {
	auto &tok = peek();
	if (tok.type == TokenType::NO_TOKEN)
		return m_toks.back().errLine;

	return tok.errLine;
}