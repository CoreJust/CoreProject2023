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


SymbolLoader::SymbolLoader(std::vector<Token>& toks, ModuleQualities qualities, const std::string& path)
	: m_toks(toks), m_qualities(qualities), m_path(path) {

}

void SymbolLoader::loadSymbols() {
	while (m_pos < m_toks.size()) {
		readAnnotations();

		// read the symbol
		if (match(TokenType::USE)) {
			loadUse();
		} else if (match(TokenType::DEF)) {
			loadFunction();
		} else if (m_toks[m_pos].type == TokenType::ABSTRACT
			|| (m_toks[m_pos].type >= TokenType::CLASS && m_toks[m_pos].type <= TokenType::UNION)) {
			loadClass();
		} else if (match(TokenType::TYPE)) {
			loadTypeVariable();
		} else {
			loadVariable();
		}

		// skip code blocks
		if (m_pos < m_toks.size() && match(TokenType::LBRACE)) {
			skipCodeInBraces();
		}
	}

	g_symbolTable.addModuleSymbols(m_path, std::move(m_symbols));
}

// TODO: implement
void SymbolLoader::loadUse() {
	skipAssignment();
}

// TODO: implement
void SymbolLoader::loadClass() {

}

void SymbolLoader::loadFunction() {
	// set default values
	FunctionQualities qualities;
	qualities.setVisibility(m_qualities.getVisibility());
	qualities.setSafety(m_qualities.getSafety());
	qualities.setMangling(m_qualities.isManglingOn());

	// read annotations
	for (auto& a : m_annots) {
		if (a[0] == "public")				qualities.setVisibility(Visibility::PUBLIC);
		else if (a[0] == "direct_import")	qualities.setVisibility(Visibility::DIRECT_IMPORT);
		else if (a[0] == "private")			qualities.setVisibility(Visibility::PRIVATE);
		else if (a[0] == "safe")			qualities.setSafety(Safety::SAFE);
		else if (a[0] == "safe_only")		qualities.setSafety(Safety::SAFE_ONLY);
		else if (a[0] == "unsafe")			qualities.setSafety(Safety::UNSAFE);
		else if (a[0] == "explicit")		qualities.setImplicit(false);
		else if (a[0] == "implicit")		qualities.setImplicit(true);
		else if (a[0] == "ccall")			qualities.setCallingConvention(CallingConvention::CCALL);
		else if (a[0] == "stdcall")			qualities.setCallingConvention(CallingConvention::STDCALL);
		else if (a[0] == "fastcall")		qualities.setCallingConvention(CallingConvention::FASTCALL);
		else if (a[0] == "thiscall")		qualities.setCallingConvention(CallingConvention::THISCALL);
		else if (a[0] == "vectorcall")		qualities.setCallingConvention(CallingConvention::VECTORCALL);
		else if (a[0] == "coldcall")		qualities.setCallingConvention(CallingConvention::COLDCALL);
		else if (a[0] == "tailcall")		qualities.setCallingConvention(CallingConvention::TAILCALL);
		else if (a[0] == "nomangle")		qualities.setMangling(false);
		else if (a[0] == "mangle")			qualities.setMangling(true);
		else if (a[0] == "noreturn")		qualities.setNoReturn(true);
		else if (a[0] == "noexcept")		qualities.setNoExcept(true);
		else ErrorManager::lexerError(
			ErrorID::E1051_UNKNOWN_ANNOTATION,
			getCurrLine(),
			"unknown function annotation: " + a[0]
		);
	}

	// read function declaration
	std::string alias = "";

	if (match(TokenType::NATIVE)) {
		qualities.setNative(true);
	}

	if (match(TokenType::WORD)) { // common function
		alias = m_toks[m_pos - 1].data;
	} else if (DEFINABLE_OPERATORS.contains(m_toks[m_pos].type)) { // operator-functions
		alias = m_toks[m_pos].data;
		if (m_toks[m_pos].type == TokenType::LPAR || m_toks[m_pos].type == TokenType::LBRACKET) {
			m_pos++;
		}

		m_pos++;
	} else { } // type conversions

	// Arguments
	std::vector<Argument> args;
	bool isVaArgs = false;

	consume(TokenType::LPAR);
	if (!match(TokenType::RPAR)) {
		do {
			if (match(TokenType::ETCETERA)) {
				if (peek().type != TokenType::RPAR) {
					ErrorManager::parserError(
						ErrorID::E2105_VA_ARGS_MUST_BE_THE_LAST_ARGUMENT,
						getCurrLine(),
						"incorrect va_args while parsing function " + alias
					);
				}

				isVaArgs = true;
			} else {
				std::unique_ptr<Type> type = TypeParser(m_toks, m_pos).consumeType();
				consume(TokenType::WORD);
				args.push_back(Argument{ m_toks[m_pos - 1].data, std::move(type) });
			}
		} while (match(TokenType::COMMA));

		consume(TokenType::RPAR);
	}

	std::unique_ptr<Type> returnType = TypeParser(m_toks, m_pos).parseTypeOrGetNoType();

	// Save results
	switch (qualities.getVisibility()) {
		case Visibility::LOCAL: break;
		case Visibility::PUBLIC:
			m_symbols.publicSymbols.addFunction(
				FunctionPrototype(
					alias,
					std::move(returnType),
					std::move(args),
					qualities,
					isVaArgs
				)
			);
			break;
		case Visibility::DIRECT_IMPORT:
			m_symbols.publicOnceSymbols.addFunction(
				FunctionPrototype(
					alias,
					std::move(returnType),
					std::move(args),
					qualities,
					isVaArgs
				)
			);
			break;
		case Visibility::PRIVATE:
			m_symbols.privateSymbols.addFunction(
				FunctionPrototype(
					alias,
					std::move(returnType),
					std::move(args),
					qualities,
					isVaArgs
				)
			);
			break;
	default: break;
	}

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
	// set default qualities
	VariableQualities qualities;
	qualities.setVisibility(m_qualities.getVisibility());
	qualities.setSafety(m_qualities.getSafety());

	// read annotations
	for (auto& a : m_annots) {
		if (a[0] == "public")				qualities.setVisibility(Visibility::PUBLIC);
		else if (a[0] == "direct_import")	qualities.setVisibility(Visibility::DIRECT_IMPORT);
		else if (a[0] == "private")			qualities.setVisibility(Visibility::PRIVATE);
		else if (a[0] == "safe")			qualities.setSafety(Safety::SAFE);
		else if (a[0] == "safe_only")		qualities.setSafety(Safety::SAFE_ONLY);
		else if (a[0] == "unsafe")			qualities.setSafety(Safety::UNSAFE);
		else if (a[0] == "thread_local")	qualities.setThreadLocal(true);
		else ErrorManager::lexerError(
			ErrorID::E1051_UNKNOWN_ANNOTATION,
			getCurrLine(),
			"unknown function annotation: " + a[0]
		);
	}

	// read variable declaration
	if (match(TokenType::CONST)) {
		qualities.setVariableType(VariableType::CONST);
	} else if (match(TokenType::EXTERN)) {
		qualities.setVariableType(VariableType::EXTERN);
	}

	std::unique_ptr<Type> type = TypeParser(m_toks, m_pos).consumeType();
	if (qualities.getVariableType() == VariableType::CONST) {
		type->isConst = true;
	}

	consume(TokenType::WORD);
	std::string alias = m_toks[m_pos - 1].data;

	// Store the variable
	switch (qualities.getVisibility()) {
		case Visibility::LOCAL: break;
		case Visibility::PUBLIC:
			m_symbols.publicSymbols.addVariable(alias, std::move(type), qualities, nullptr);
			break;
		case Visibility::DIRECT_IMPORT:
			m_symbols.publicOnceSymbols.addVariable(alias, std::move(type), qualities, nullptr);
			break;
		case Visibility::PRIVATE:
			m_symbols.privateSymbols.addVariable(alias, std::move(type), qualities, nullptr);
			break;
	default: break;
	}

	// skip code
	if (match(TokenType::EQ)) {
		skipAssignment();
	} else {
		consume(TokenType::SEMICOLON);
	}
}

void SymbolLoader::loadTypeVariable() {
	// set default qualities
	TypeQualities qualities;
	qualities.setVisibility(m_qualities.getVisibility());
	qualities.setSafety(m_qualities.getSafety());

	// read annotations
	for (auto& a : m_annots) {
		if (a[0] == "public")				qualities.setVisibility(Visibility::PUBLIC);
		else if (a[0] == "direct_import")	qualities.setVisibility(Visibility::DIRECT_IMPORT);
		else if (a[0] == "private")			qualities.setVisibility(Visibility::PRIVATE);
		else if (a[0] == "safe")			qualities.setSafety(Safety::SAFE);
		else if (a[0] == "safe_only")		qualities.setSafety(Safety::SAFE_ONLY);
		else if (a[0] == "unsafe")			qualities.setSafety(Safety::UNSAFE);
		else ErrorManager::lexerError(
			ErrorID::E1051_UNKNOWN_ANNOTATION,
			getCurrLine(),
			"unknown function annotation: " + a[0]
		);
	}

	std::string alias = consume(TokenType::WORD).data;
	consume(TokenType::EQ);

	std::unique_ptr<Type> type = TypeParser(m_toks, m_pos).consumeType();
	consume(TokenType::SEMICOLON);

	llvm::Type* llvmType = type->to_llvm();
	std::shared_ptr<TypeNode> typeNode = std::make_shared<TypeNode>(alias, TypeQualities(), std::move(type), llvmType);

	// Store the variable
	switch (qualities.getVisibility()) {
		case Visibility::PUBLIC:
			m_symbols.publicSymbols.addType(std::move(typeNode));
			break;
		case Visibility::DIRECT_IMPORT:
			m_symbols.publicOnceSymbols.addType(std::move(typeNode));
			break;
		case Visibility::PRIVATE:
			m_symbols.privateSymbols.addType(std::move(typeNode));
			break;
	default: break;
	}
}

void SymbolLoader::skipCodeInBraces() {
	i32 depth = 1;
	while (depth) {
		if (m_pos >= m_toks.size()) {
			ErrorManager::lexerError(
				ErrorID::E1004_NO_CLOSING_BRACE,
				getCurrLine(),
				""
			);
		}

		if (match(TokenType::LBRACE)) {
			depth++;
		} else if (match(TokenType::RBRACE)) {
			depth--;
		} else {
			m_pos++;
		}
	}
}

void SymbolLoader::skipAssignment() {
	while (true) {
		if (m_pos >= m_toks.size()) {
			ErrorManager::lexerError(
				ErrorID::E1004_NO_CLOSING_BRACE,
				getCurrLine(),
				""
			);
		}

		if (match(TokenType::SEMICOLON)) {
			break;
		} else if (match(TokenType::LBRACE)) {
			skipCodeInBraces();
		}

		m_pos++;
	}
}

void SymbolLoader::readAnnotations() {
	m_annots.clear();
	while (m_toks[m_pos].type == TokenType::AT) {
		auto line = m_toks[m_pos].errLine;
		m_pos++;
		m_annots.push_back(std::vector<std::string>());

		while (m_pos < m_toks.size() && m_toks[m_pos].errLine == line) {
			m_annots.back().push_back(m_toks[m_pos++].data);
		}
	}
}

Token& SymbolLoader::consume(TokenType type) {
	if (!match(type)) {
		ErrorManager::parserError(
			ErrorID::E2002_UNEXPECTED_TOKEN,
			getCurrLine(),
			"expected " + Token::toString(type)
		);
	}

	return peek(-1);
}

bool SymbolLoader::match(TokenType type) {
	if (peek().type != type) {
		return false;
	}

	m_pos++;
	return true;
}

Token& SymbolLoader::peek(int rel) {
	static Token _NO_TOK = Token();
	u64 pos = m_pos + rel;
	if (pos >= m_toks.size()) {
		return _NO_TOK;
	}

	return m_toks[pos];
}

int SymbolLoader::getCurrLine() {
	Token& tok = peek();
	if (tok.type == TokenType::NO_TOKEN) {
		return m_toks.back().errLine;
	}

	return tok.errLine;
}