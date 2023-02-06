#include "TypeParser.h"
#include <Utils/ErrorManager.h>
#include <Module/Module.h>

static Token _NO_TOK = Token();

TypeParser::TypeParser(std::vector<Token>& toks, u64& pos)
	: m_toks(toks), m_pos(pos), m_originalPos(pos) {

}

std::unique_ptr<Type> TypeParser::consumeType() {
	if (auto type = parseType()) {
		return std::move(type);
	}

	ErrorManager::typeError(ErrorID::E3001_TYPE_NOT_SPECIFIED, getCurrLine(), "");
	return nullptr;
}

std::unique_ptr<Type> TypeParser::parseTypeOrGetNoType() {
	if (auto type = parseType()) {
		return std::move(type);
	}

	return std::make_unique<Type>();
}

std::unique_ptr<Type> TypeParser::parseType() {
	std::unique_ptr<Type> result;

	// First part: type itself
	m_pos++;
	switch (peek(-1).type) {
		case TokenType::BOOL: result = std::make_unique<Type>(BasicType::BOOL, false); break;
		case TokenType::C8: result = std::make_unique<Type>(BasicType::C8, false); break;
		case TokenType::C16: result = std::make_unique<Type>(BasicType::C16, false); break;
		case TokenType::C32: result = std::make_unique<Type>(BasicType::C32, false); break;
		case TokenType::STR8: result = std::make_unique<Type>(BasicType::STR8, false); break;
		case TokenType::STR16: result = std::make_unique<Type>(BasicType::STR16, false); break;
		case TokenType::STR32: result = std::make_unique<Type>(BasicType::STR32, false); break;
		case TokenType::I8: result = std::make_unique<Type>(BasicType::I8, false); break;
		case TokenType::I16: result = std::make_unique<Type>(BasicType::I16, false); break;
		case TokenType::I32: result = std::make_unique<Type>(BasicType::I32, false); break;
		case TokenType::I64: result = std::make_unique<Type>(BasicType::I64, false); break;
		case TokenType::U8: result = std::make_unique<Type>(BasicType::U8, false); break;
		case TokenType::U16: result = std::make_unique<Type>(BasicType::U16, false); break;
		case TokenType::U32: result = std::make_unique<Type>(BasicType::U32, false); break;
		case TokenType::U64: result = std::make_unique<Type>(BasicType::U64, false); break;
		case TokenType::F32: result = std::make_unique<Type>(BasicType::F32, false); break;
		case TokenType::F64: result = std::make_unique<Type>(BasicType::F64, false); break;

		case TokenType::FUNC: {
			std::unique_ptr<Type> returnType = parseTypeOrGetNoType();
			std::vector<std::unique_ptr<Type>> argTypes;
			consume(TokenType::LPAR);
			if (!match(TokenType::RPAR)) {
				do {
					argTypes.push_back(consumeType());
				} while (match(TokenType::COMMA));
				consume(TokenType::RPAR);
			}

			return std::make_unique<FunctionType>(std::move(returnType), std::move(argTypes), false);
		}; break;
		case TokenType::TUPLE: {
			std::vector<std::unique_ptr<Type>> subTypes;
			consume(TokenType::LESS);
			if (!match(TokenType::BIGGER)) {
				do {
					subTypes.push_back(consumeType());
				} while (match(TokenType::COMMA));
				consume(TokenType::BIGGER);
			}

			return std::make_unique<TupleType>(std::move(subTypes), false);
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

				m_pos = m_originalPos;
				return nullptr;
			} else {
				// TODO: add generics/user-defined types support
				ErrorManager::warning(ErrorID::W0001, "not developed yet");
				result = std::unique_ptr<Type>(g_module->getType(moduleName, name)->type->copy());
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
				result = std::unique_ptr<Type>(g_module->getVariable(moduleName, name)->type->copy());
			} else if (symType == SymbolType::FUNCTION) {
				result = g_module->getFunction(moduleName, name)->prototype.genType();
			}

			if (hasParens) {
				consume(TokenType::RPAR);
			}
		}; break;
	default: m_pos--; m_pos = m_originalPos; return nullptr;
	}

	// second type: modificators
	while (true) {
		if (match(TokenType::LBRACKET)) {
			if (match(TokenType::RBRACKET)) { // dynamic array
				result = std::make_unique<PointerType>(BasicType::DYN_ARRAY, std::move(result), false);
				continue;
			} else { // static array
				if (!matchRange(TokenType::NUMBERI8, TokenType::NUMBERU64)) {
					ErrorManager::typeError(ErrorID::E3002_UNEXPECTED_TOKEN_WHILE_PARSING_TYPE, getCurrLine(), "expected number");
				} else if (peek(-1).data[0] == '-' || peek(-1).data == "0") {
					ErrorManager::typeError(ErrorID::E3052_NEGATIVE_SIZE_ARRAY, getCurrLine(), "array size: " + peek(-1).data);
				} else {
					u64 size = std::stoull(peek(-1).data);
					consume(TokenType::RBRACKET);
					result = std::make_unique<ArrayType>(std::move(result), size, true);
					continue;
				}
			}
		} else if (match(TokenType::QUESTION)) { // optional
			result = std::make_unique<PointerType>(BasicType::OPTIONAL, std::move(result), false);
			continue;
		} else if (match(TokenType::ANDAND)) { // rvalue reference
			if (isReference(result->basicType)) {
				ErrorManager::typeError(ErrorID::E3051_REFERENCE_TO_REFERENCE, getCurrLine(), "rvalue reference to a reference");
				break;
			}

			result = std::make_unique<PointerType>(BasicType::RVAL_REFERENCE, std::move(result), false);
			continue;
		}

		bool isConst = match(TokenType::CONST);
		if (match(TokenType::STAR)) { // pointer
			if (isReference(result->basicType)) {
				ErrorManager::typeError(ErrorID::E3051_REFERENCE_TO_REFERENCE, getCurrLine(), "pointer to a reference");
				break;
			}

			result = std::make_unique<PointerType>(BasicType::POINTER, std::move(result), isConst);
			continue;
		} else if (match(TokenType::AND)) { // reference
			if (isReference(result->basicType)) {
				ErrorManager::typeError(ErrorID::E3051_REFERENCE_TO_REFERENCE, getCurrLine(), "");
				break;
			}

			result = std::make_unique<PointerType>(BasicType::REFERENCE, std::move(result), isConst);
			continue;
		}

		break;
	}

	if (!result) {
		m_pos = m_originalPos;
	}

	return result;
}

bool TypeParser::isType() {
	if (parseType()) {
		m_pos = m_originalPos;
		return true;
	}

	return false;
}

Token& TypeParser::consume(TokenType type) {
	if (!match(type))
		ErrorManager::typeError(ErrorID::E3002_UNEXPECTED_TOKEN_WHILE_PARSING_TYPE, getCurrLine(), "expected " + Token::toString(type));

	return peek(-1);
}

bool TypeParser::match(TokenType type) {
	if (peek().type != type)
		return false;

	m_pos++;
	return true;
}

bool TypeParser::matchRange(TokenType from, TokenType to) {
	auto type = peek().type;
	if (type < from || type > to)
		return false;

	m_pos++;
	return true;
}

Token& TypeParser::next() {
	if (m_pos >= m_toks.size())
		return _NO_TOK;

	return m_toks[m_pos++];
}

Token& TypeParser::peek(int rel) {
	u64 pos = m_pos + rel;
	if (pos >= m_toks.size())
		return _NO_TOK;

	return m_toks[pos];
}

int TypeParser::getCurrLine() {
	auto& tok = peek();
	if (tok.type == TokenType::NO_TOKEN)
		return m_toks.back().errLine;

	return tok.errLine;
}
