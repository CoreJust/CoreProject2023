#include "INode.h"
#include <vector>
#include <Lexer/Token.h>

extern u64* g_pos;
extern std::vector<Token>* g_toks;

std::string INode::s_tabs = "";

INode::INode() {
	if (*g_pos >= g_toks->size()) {
		m_errLine = -1;
	} else {
		if (*g_pos > 0 && g_toks->at(*g_pos - 1).type == TokenType::SEMICOLON) {
			m_errLine = g_toks->at(*g_pos - 1).errLine;
		} else {
			m_errLine = g_toks->at(*g_pos).errLine;
		}
	}
}

u64 INode::getErrLine() const {
	return m_errLine;
}

Safety INode::getSafety() const {
	return m_safety;
}
