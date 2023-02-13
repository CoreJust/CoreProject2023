#include "SymbolPreloader.h"
#include <Utils/ErrorManager.h>
#include <Parser/TypeParser.h>

extern std::set<TokenType> DEFINABLE_OPERATORS;


SymbolPreloader::SymbolPreloader(std::vector<Token>& toks, const std::string& path)
	: BasicSymbolLoader(toks, path) {

}

void SymbolPreloader::loadUse() {
	skipAssignment();
}

void SymbolPreloader::loadClass() {
	u64 tokenPos = m_pos;

	TypeQualities qualities;
	qualities.setVisibility(g_module->getQualities().getVisibility());
	qualities.setSafety(g_module->getQualities().getSafety());
	qualities.setClassType(ClassType::COMMON);

	for (auto& a : m_annots) {
		if (a[0] == "public")				qualities.setVisibility(Visibility::PUBLIC);
		else if (a[0] == "direct_import")	qualities.setVisibility(Visibility::DIRECT_IMPORT);
		else if (a[0] == "private")			qualities.setVisibility(Visibility::PRIVATE);
		else if (a[0] == "safe")			qualities.setSafety(Safety::SAFE);
		else if (a[0] == "safe_only")		qualities.setSafety(Safety::SAFE_ONLY);
		else if (a[0] == "unsafe")			qualities.setSafety(Safety::UNSAFE);
		else if (a[0] == "move_only")		qualities.setMoveOnly(true);
		else ErrorManager::lexerError(
			ErrorID::E1051_UNKNOWN_ANNOTATION,
			getCurrLine(),
			"unknown class/struct annotation: " + a[0]
		);
	}

	if (match(TokenType::STRUCT)) {
		std::string name = consume(TokenType::WORD).data;
		consume(TokenType::LBRACE);

		while (!match(TokenType::RBRACE)) {
			if (match(TokenType::DEF)) {
				// TODO: implement method handling
			} else {
				match(TokenType::STATIC);
				TypeParser(m_toks, m_pos).skipConsumeType();
				consume(TokenType::WORD);

				if (match(TokenType::EQ)) {
					skipAssignment();
				} else {
					consume(TokenType::SEMICOLON);
				}
			}
		}

		std::shared_ptr<TypeNode> result = std::make_shared<TypeNode>(
			name,
			qualities,
			nullptr,
			nullptr
			);

		m_symbols.addType(qualities.getVisibility(), std::move(result), tokenPos);
	}
}

void SymbolPreloader::loadFunction() {
	u64 tokenPos = m_pos;

	// set default values
	FunctionQualities qualities;
	qualities.setVisibility(g_module->getQualities().getVisibility());
	qualities.setSafety(g_module->getQualities().getSafety());
	qualities.setMangling(g_module->getQualities().isManglingOn());

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
	} else {} // type conversions

	// Arguments
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
			} else {
				TypeParser(m_toks, m_pos).skipConsumeType();
				consume(TokenType::WORD);
			}
		} while (match(TokenType::COMMA));

		consume(TokenType::RPAR);
	}

	TypeParser(m_toks, m_pos).skipType();

	// Save results
	m_symbols.addFunction(
		qualities.getVisibility(),
		FunctionPrototype(
			alias,
			nullptr,
			{ },
			qualities,
			false
		),
		tokenPos
	);

	// skip code
	if (match(TokenType::EQ)) {
		skipAssignment();
	} else if (match(TokenType::LBRACE)) {
		skipCodeInBraces();
	} else {
		consume(TokenType::SEMICOLON);
	}
}

void SymbolPreloader::loadVariable() {
	u64 tokenPos = m_pos;

	// set default qualities
	VariableQualities qualities;
	qualities.setVisibility(g_module->getQualities().getVisibility());
	qualities.setSafety(g_module->getQualities().getSafety());

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
			"unknown variable annotation: " + a[0]
		);
	}

	// read variable declaration
	if (match(TokenType::CONST)) {
		qualities.setVariableType(VariableType::CONST);
	} else if (match(TokenType::EXTERN)) {
		qualities.setVariableType(VariableType::EXTERN);
	}

	TypeParser(m_toks, m_pos).skipConsumeType();
	std::string alias = consume(TokenType::WORD).data;

	m_symbols.addVariable(qualities.getVisibility(), alias, qualities, tokenPos);

	// skip code
	if (match(TokenType::EQ)) {
		skipAssignment();
	} else {
		consume(TokenType::SEMICOLON);
	}
}

void SymbolPreloader::loadTypeVariable() {
	u64 tokenPos = m_pos;

	// set default qualities
	TypeQualities qualities;
	qualities.setVisibility(g_module->getQualities().getVisibility());
	qualities.setSafety(g_module->getQualities().getSafety());

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
			"unknown type annotation: " + a[0]
		);
	}

	std::string alias = consume(TokenType::WORD).data;
	consume(TokenType::EQ);

	TypeParser(m_toks, m_pos).skipConsumeType();
	consume(TokenType::SEMICOLON);

	std::shared_ptr<TypeNode> typeNode = std::make_shared<TypeNode>(alias, TypeQualities(), nullptr, nullptr);
	m_symbols.addType(qualities.getVisibility(), std::move(typeNode), tokenPos);
}
