#include "BasicSymbolLoader.h"
#include <Utils/ErrorManager.h>


BasicSymbolLoader::BasicSymbolLoader(std::vector<Token>& toks, const std::string& path)
	: BasicParser(toks, m_truePos), m_path(path), m_symbols(g_module->getOwnSymbols()) {

}

void BasicSymbolLoader::loadSymbols() {
	while (m_pos < m_toks.size()) {
		readAnnotations();

		// read the symbol
		if (match(TokenType::USE)) {
			this->loadUse();
		} else if (match(TokenType::DEF)) {
			this->loadFunction();
		} else if (m_toks[m_pos].type == TokenType::ABSTRACT
			|| (m_toks[m_pos].type >= TokenType::CLASS && m_toks[m_pos].type <= TokenType::UNION)) {
			this->loadClass();
		} else if (match(TokenType::TYPE)) {
			this->loadTypeVariable();
		} else {
			this->loadVariable();
		}

		// skip code blocks
		if (m_pos < m_toks.size() && match(TokenType::LBRACE)) {
			skipCodeInBraces();
		}
	}
}

void BasicSymbolLoader::skipCodeInBraces() {
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
		}
		else if (match(TokenType::RBRACE)) {
			depth--;
		}
		else {
			m_pos++;
		}
	}
}

void BasicSymbolLoader::skipAssignment() {
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
		}
		else if (match(TokenType::LBRACE)) {
			skipCodeInBraces();
		}

		m_pos++;
	}
}

void BasicSymbolLoader::readAnnotations() {
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
