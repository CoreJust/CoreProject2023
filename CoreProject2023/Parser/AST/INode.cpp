#include "INode.h"
#include <vector>
#include <Lexer/Token.h>

extern u64* g_pos;
extern std::vector<Token>* g_toks;

INode::INode() {
	if (*g_pos >= g_toks->size()) {
		m_errLine = -1;
	} else {
		m_errLine = g_toks->at(*g_pos).errLine;
	}
}

u64 INode::getErrLine() const {
	return m_errLine;
}