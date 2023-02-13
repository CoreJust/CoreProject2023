#include "SymbolLoader.h"
#include <set>
#include <Utils/ErrorManager.h>
#include <Parser/TypeParser.h>

std::set<TokenType> DEFINABLE_OPERATORS = {
	TokenType::IN, TokenType::IS, TokenType::EQ,
	TokenType::PLUS_EQ, TokenType::MINUS_EQ, TokenType::STAR_EQ,
	TokenType::SLASH_EQ, TokenType::DSLASH_EQ, TokenType::PERCENT_EQ,
	TokenType::POWER_EQ, TokenType::AND_EQ, TokenType::OR_EQ,
	TokenType::XOR_EQ, TokenType::LSHIFT_EQ, TokenType::RSHIFT_EQ,
	TokenType::INCREMENT, TokenType::DECREMENT,

	TokenType::PLUS, TokenType::MINUS, TokenType::STAR,
	TokenType::SLASH, TokenType::DSLASH, TokenType::PERCENT,
	TokenType::POWER,

	TokenType::AND, TokenType::OR, TokenType::XOR,
	TokenType::LSHIFT, TokenType::RSHIFT, TokenType::TILDE,

	TokenType::EXCL, TokenType::EXCLEQ, TokenType::LESS,
	TokenType::GREATER, TokenType::EQEQ, TokenType::LESSEQ,
	TokenType::GREATEREQ,

	TokenType::ANDAND, TokenType::OROR,

	TokenType::LPAR, // ()
	TokenType::LBRACKET, // []

	TokenType::RANGEDOT,
	TokenType::ETCETERA,
};


SymbolLoader::SymbolLoader(std::vector<Token>& toks, const std::string& path)
	: BasicSymbolLoader(toks, path) {

}

void SymbolLoader::loadUse() {
	// TODO: implement here
	skipAssignment();
}

// TODO: implement
void SymbolLoader::loadClass() {
	TypeNode* typeNode = m_symbols.getType(m_pos).get();

	if (match(TokenType::STRUCT)) {
		std::string name = consume(TokenType::WORD).data;
		std::vector<std::unique_ptr<Type>> fieldTypes;
		consume(TokenType::LBRACE);

		std::vector<Variable> fields;
		std::vector<Function> methods;
		while (!match(TokenType::RBRACE)) {
			if (match(TokenType::DEF)) {
				// TODO: implement method handling
			} else {
				Visibility visibility = Visibility::PUBLIC;
				if (match(TokenType::PUBLIC)) {
					visibility = Visibility::PUBLIC;
				} else if (match(TokenType::PROTECTED)) {
					visibility = Visibility::DIRECT_IMPORT;
				} else if (match(TokenType::PRIVATE)) {
					visibility = Visibility::PRIVATE;
				}

				bool isStatic = match(TokenType::STATIC);
				std::unique_ptr<Type> type = TypeParser(m_toks, m_pos).consumeType();
				std::string fieldName = consume(TokenType::WORD).data;

				if (match(TokenType::EQ)) {
					skipAssignment();
				} else {
					consume(TokenType::SEMICOLON);
				}

				VariableQualities fieldQualities;
				fieldQualities.setSafety(typeNode->qualities.getSafety());
				fieldQualities.setVisibility(visibility);
				fieldQualities.setVariableType(VariableType::FIELD);

				fieldTypes.push_back(type->copy());
				fields.push_back(Variable(fieldName, std::move(type), fieldQualities, nullptr));
			}
		}

		std::unique_ptr<StructType> type = std::make_unique<StructType>(std::move(fieldTypes));
		typeNode->llvmType = type->to_llvm();
		typeNode->type = std::move(type);
		typeNode->fields = std::move(fields);
		typeNode->methods = std::move(methods);
	}
}

void SymbolLoader::loadFunction() {
	Function* func = m_symbols.getFunction(m_pos);

	// read function declaration
	match(TokenType::NATIVE);

	if (!match(TokenType::WORD)) {
		if (DEFINABLE_OPERATORS.contains(m_toks[m_pos].type)) { // operator-functions
			if (m_toks[m_pos].type == TokenType::LPAR || m_toks[m_pos].type == TokenType::LBRACKET) {
				m_pos++;
			}

			m_pos++;
		} else {} // type conversions
	}

	// Arguments
	std::vector<Argument> args;

	consume(TokenType::LPAR);
	if (!match(TokenType::RPAR)) {
		do {
			if (match(TokenType::ETCETERA)) {
				if (peek().type != TokenType::RPAR) {
					ErrorManager::parserError(
						ErrorID::E2105_VA_ARGS_MUST_BE_THE_LAST_ARGUMENT,
						getCurrLine(),
						"incorrect va_args while parsing function " + func->prototype.getName()
					);
				}

				func->prototype.setVaArgs(true);
			} else {
				std::unique_ptr<Type> type = TypeParser(m_toks, m_pos).consumeType();
				consume(TokenType::WORD);
				args.push_back(Argument{ m_toks[m_pos - 1].data, std::move(type) });
			}
		} while (match(TokenType::COMMA));

		consume(TokenType::RPAR);
	}

	func->prototype.args() = std::move(args);
	func->prototype.getReturnType() = TypeParser(m_toks, m_pos).parseTypeOrGetNoType();

	// skip code
	if (match(TokenType::EQ)) {
		skipAssignment();
	} else if (match(TokenType::LBRACE)) {
		skipCodeInBraces();
	} else {
		consume(TokenType::SEMICOLON);
	}
}

void SymbolLoader::loadVariable() {
	Variable* var = m_symbols.getVariable(m_pos);

	// read variable declaration
	if (!match(TokenType::CONST)) {
		match(TokenType::EXTERN);
	}

	var->type = TypeParser(m_toks, m_pos).consumeType();
	if (var->qualities.getVariableType() == VariableType::CONST) {
		var->type->isConst = true;
	}

	consume(TokenType::WORD);

	// skip code
	if (match(TokenType::EQ)) {
		skipAssignment();
	} else {
		consume(TokenType::SEMICOLON);
	}
}

void SymbolLoader::loadTypeVariable() {
	TypeNode* typeNode = m_symbols.getType(m_pos).get();

	std::string alias = consume(TokenType::WORD).data;
	consume(TokenType::EQ);

	typeNode->type = TypeParser(m_toks, m_pos).consumeType();
	consume(TokenType::SEMICOLON);

	typeNode->llvmType = typeNode->type->to_llvm();
}
