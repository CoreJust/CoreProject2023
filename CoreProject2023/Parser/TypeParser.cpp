#include "TypeParser.h"
#include <Utils/ErrorManager.h>
#include <Module/Module.h>

static Token _NO_TOK = Token();

TypeParser::TypeParser(std::vector<Token>& toks, u64& pos)
	: BasicParser(toks, pos) {

}

void TypeParser::skipConsumeType() {
	if (skipType()) {
		return;
	}

	ErrorManager::typeError(
		ErrorID::E3001_TYPE_NOT_SPECIFIED,
		getCurrLine(),
		""
	);
}

bool TypeParser::skipType() {
	match(TokenType::CONST);

	m_pos++;
	if (peek(-1).type < TokenType::BOOL || peek(-1).type > TokenType::F64) {
		switch (peek(-1).type) {
		case TokenType::FUNC: {
			skipType();
			consume(TokenType::LPAR);
			if (!match(TokenType::RPAR)) {
				do {
					if (match(TokenType::ETCETERA)) {
						if (peek().type != TokenType::RPAR) {
							ErrorManager::parserError(
								ErrorID::E2105_VA_ARGS_MUST_BE_THE_LAST_ARGUMENT,
								getCurrLine(),
								"incorrect va_args while parsing a function-type"
							);
						}
					} else {
						skipConsumeType();
					}
				} while (match(TokenType::COMMA));
				consume(TokenType::RPAR);
			}
		}; break;
		case TokenType::TUPLE: {
			consume(TokenType::LESS);
			if (!match(TokenType::GREATER)) {
				do {
					skipConsumeType();
				} while (match(TokenType::COMMA));
				consume(TokenType::GREATER);
			}
		}; break;
		case TokenType::STRUCT: {
			consume(TokenType::LBRACE);
			while (!match(TokenType::RBRACE)) {
				skipConsumeType();
				consume(TokenType::SEMICOLON);
			}
		}; break;

		case TokenType::WORD: {
			while (match(TokenType::DOT)) {
				consume(TokenType::WORD);
			}
		}; break;
		case TokenType::TYPEOF: {
			bool hasParens = match(TokenType::LPAR);
			consume(TokenType::WORD);
			while (match(TokenType::DOT)) {
				if (peek().type > TokenType::NUMBERU64 && peek().type != TokenType::WORD) {
					ErrorManager::parserError(
						ErrorID::E2002_UNEXPECTED_TOKEN,
						getCurrLine(),
						"expected WORD or NUMBER"
					);
				} else {
					next();
				}
			}

			if (hasParens) {
				consume(TokenType::RPAR);
			}
		}; break;
		default: m_pos--; return false;
		}
	}

	// second type: modificators
	while (true) {
		match(TokenType::CONST);
		if (match(TokenType::LBRACKET)) {
			if (match(TokenType::RBRACKET)) { // dynamic array
				continue;
			} else { // static array
				if (!matchRange(TokenType::NUMBERI8, TokenType::NUMBERU64)) {
					ErrorManager::typeError(
						ErrorID::E3002_UNEXPECTED_TOKEN_WHILE_PARSING_TYPE,
						getCurrLine(),
						"expected a number"
					);
				} else if (peek(-1).data[0] == '-' || peek(-1).data == "0") {
					ErrorManager::typeError(
						ErrorID::E3052_NEGATIVE_SIZE_ARRAY,
						getCurrLine(),
						"array size: " + peek(-1).data
					);
				} else {
					consume(TokenType::RBRACKET);
					continue;
				}
			}
		} else if (match(TokenType::QUESTION)) { // optional
			continue;
		} else if (match(TokenType::ANDAND)) { // rvalue reference
			continue;
		}

		if (match(TokenType::STAR)) { // pointer
			continue;
		} else if (match(TokenType::POWER)) { // pointer to pointer
			continue;
		} else if (match(TokenType::AND)) { // reference
			continue;
		}

		break;
	}

	return true;
}

std::shared_ptr<Type> TypeParser::consumeType() {
	if (auto type = parseType()) {
		return std::move(type);
	}

	ErrorManager::typeError(
		ErrorID::E3001_TYPE_NOT_SPECIFIED,
		getCurrLine(), 
		""
	);

	return nullptr;
}

std::shared_ptr<Type> TypeParser::parseTypeOrGetNoType() {
	savePos();
	if (auto type = parseType()) {
		return std::move(type);
	}

	loadPos();
	return Type::createType(BasicType::NO_TYPE);
}

std::shared_ptr<Type> TypeParser::tryParseType() {
	savePos();
	if (auto type = parseType()) {
		return std::move(type);
	}

	loadPos();
	return nullptr;
}

std::shared_ptr<Type> TypeParser::parseType() {
	std::shared_ptr<Type> result;

	// First part: type itself
	bool isConst = match(TokenType::CONST);

	m_pos++;
	switch (peek(-1).type) {
		case TokenType::BOOL: result = Type::createType(BasicType::BOOL, isConst); break;
		case TokenType::C8: result = Type::createType(BasicType::C8, isConst); break;
		case TokenType::C16: result = Type::createType(BasicType::C16, isConst); break;
		case TokenType::C32: result = Type::createType(BasicType::C32, isConst); break;
		case TokenType::STR8: result = Type::createType(BasicType::STR8, isConst); break;
		case TokenType::STR16: result = Type::createType(BasicType::STR16, isConst); break;
		case TokenType::STR32: result = Type::createType(BasicType::STR32, isConst); break;
		case TokenType::I8: result = Type::createType(BasicType::I8, isConst); break;
		case TokenType::I16: result = Type::createType(BasicType::I16, isConst); break;
		case TokenType::I32: result = Type::createType(BasicType::I32, isConst); break;
		case TokenType::I64: result = Type::createType(BasicType::I64, isConst); break;
		case TokenType::U8: result = Type::createType(BasicType::U8, isConst); break;
		case TokenType::U16: result = Type::createType(BasicType::U16, isConst); break;
		case TokenType::U32: result = Type::createType(BasicType::U32, isConst); break;
		case TokenType::U64: result = Type::createType(BasicType::U64, isConst); break;
		case TokenType::F32: result = Type::createType(BasicType::F32, isConst); break;
		case TokenType::F64: result = Type::createType(BasicType::F64, isConst); break;

		case TokenType::FUNC: {
			std::shared_ptr<Type> returnType = parseTypeOrGetNoType();
			std::vector<std::shared_ptr<Type>> argTypes;
			bool isVaArgs = false;

			consume(TokenType::LPAR);
			if (!match(TokenType::RPAR)) {
				do {
					if (match(TokenType::ETCETERA)) {
						if (peek().type != TokenType::RPAR) {
							ErrorManager::parserError(
								ErrorID::E2105_VA_ARGS_MUST_BE_THE_LAST_ARGUMENT,
								getCurrLine(), 
								"incorrect va_args while parsing a function-type"
							);
						}

						isVaArgs = true;
					} else {
						argTypes.push_back(consumeType());
					}
				} while (match(TokenType::COMMA));
				consume(TokenType::RPAR);
			}

			result = FunctionType::createType(std::move(returnType), std::move(argTypes), isVaArgs, isConst);
		}; break;
		case TokenType::TUPLE: {
			std::vector<std::shared_ptr<Type>> subTypes;
			consume(TokenType::LESS);
			if (!match(TokenType::GREATER)) {
				do {
					subTypes.push_back(consumeType());
				} while (match(TokenType::COMMA));
				consume(TokenType::GREATER);
			}

			result = TupleType::createType(std::move(subTypes), isConst);
		}; break;
		case TokenType::STRUCT: {
			std::vector<std::shared_ptr<Type>> fieldTypes;
			consume(TokenType::LBRACE);
			while (!match(TokenType::RBRACE)) {
				fieldTypes.push_back(consumeType());
				consume(TokenType::SEMICOLON);
			}

			result = StructType::createType(std::move(fieldTypes), isConst);
		}; break;

		case TokenType::WORD: {
			std::string name = peek(-1).data;
			std::string moduleName = "";
			SymbolType symType = g_module->getSymbolType(name);
			if (symType == SymbolType::MODULE) {
				consume(TokenType::DOT);
				moduleName = std::move(name);
				name = consume(TokenType::WORD).data;
				symType = g_module->getSymbolType(moduleName, name);
			}

			if (symType != SymbolType::TYPE) {
				if (moduleName.size()) {
					m_pos -= 2;
				}

				return nullptr;
			} else {
				std::shared_ptr<TypeNode> typeNode = g_module->getType(moduleName, name);
				result = TypeNode::genType(std::move(typeNode), isConst);
			}
		}; break;
		case TokenType::TYPEOF: {
			bool hasParens = match(TokenType::LPAR);
			std::string name = consume(TokenType::WORD).data;
			std::string moduleName = "";
			SymbolType symType = g_module->getSymbolType(name);
			if (symType == SymbolType::MODULE) {
				consume(TokenType::DOT);
				moduleName = std::move(name);
				name = consume(TokenType::WORD).data;
				symType = g_module->getSymbolType(moduleName, name);
			}

			if (symType == SymbolType::VARIABLE) {
				result = std::shared_ptr<Type>(g_module->getVariable(moduleName, name)->type);
			} else if (symType == SymbolType::FUNCTION) {
				result = g_module->getFunction(moduleName, name)->prototype.genType();
			}

			if (hasParens) {
				consume(TokenType::RPAR);
			}
		}; break;
	default: m_pos--; return nullptr;
	}

	// second type: modificators
	while (true) {
		bool isConst = match(TokenType::CONST);
		if (match(TokenType::LBRACKET)) {
			if (match(TokenType::RBRACKET)) { // dynamic array
				result = PointerType::createType(BasicType::DYN_ARRAY, std::move(result), isConst);
				continue;
			} else { // static array
				if (!matchRange(TokenType::NUMBERI8, TokenType::NUMBERU64)) {
					ErrorManager::typeError(
						ErrorID::E3002_UNEXPECTED_TOKEN_WHILE_PARSING_TYPE,
						getCurrLine(),
						"expected a number"
					);
				} else if (peek(-1).data[0] == '-' || peek(-1).data == "0") {
					ErrorManager::typeError(
						ErrorID::E3052_NEGATIVE_SIZE_ARRAY,
						getCurrLine(),
						"array size: " + peek(-1).data
					);
				} else {
					u64 size = std::stoull(peek(-1).data);
					consume(TokenType::RBRACKET);
					result = ArrayType::createType(std::move(result), size, isConst);
					continue;
				}
			}
		} else if (match(TokenType::QUESTION)) { // optional
			result = PointerType::createType(BasicType::OPTIONAL, std::move(result), isConst);
			continue;
		} else if (match(TokenType::ANDAND)) { // rvalue reference
			if (isReference(result->basicType)) {
				ErrorManager::typeError(
					ErrorID::E3051_REFERENCE_TO_REFERENCE,
					getCurrLine(),
					"rvalue reference to a reference"
				);

				break;
			}

			result = PointerType::createType(BasicType::RVAL_REFERENCE, std::move(result), false);
			continue;
		}

		if (match(TokenType::STAR)) { // pointer
			if (isReference(result->basicType)) {
				ErrorManager::typeError(
					ErrorID::E3051_REFERENCE_TO_REFERENCE,
					getCurrLine(),
					"pointer to a reference"
				);

				break;
			}

			result = PointerType::createType(BasicType::POINTER, std::move(result), isConst);
			continue;
		} else if (match(TokenType::POWER)) { // pointer to pointer
			if (isReference(result->basicType)) {
				ErrorManager::typeError(
					ErrorID::E3051_REFERENCE_TO_REFERENCE,
					getCurrLine(),
					"pointer to a reference"
				);

				break;
			}

			result = PointerType::createType(BasicType::POINTER, std::move(result), isConst);
			result = PointerType::createType(BasicType::POINTER, std::move(result), false);
			continue;
		} else if (match(TokenType::AND)) { // reference
			if (isReference(result->basicType)) {
				ErrorManager::typeError(
					ErrorID::E3051_REFERENCE_TO_REFERENCE,
					getCurrLine(),
					""
				);

				break;
			}

			result = PointerType::createType(BasicType::LVAL_REFERENCE, std::move(result), isConst);
			continue;
		}

		break;
	}

	return result;
}

bool TypeParser::isType(bool isFalseOnDot) {
	savePos();
	if (parseType()) {
		if (!isFalseOnDot || !match(TokenType::DOT)) {
			loadPos();
			return true;
		}
	}

	loadPos();
	return false;
}

void TypeParser::savePos() {
	m_posHistory.push_back(m_pos);
}

void TypeParser::loadPos() {
	m_pos = m_posHistory.back();
	m_posHistory.pop_back();
}
