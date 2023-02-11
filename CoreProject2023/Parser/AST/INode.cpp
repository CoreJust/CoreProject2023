#include "INode.h"

extern u64* g_errLine;

INode::INode()
	: m_errLine(*g_errLine) {

}

u64 INode::getErrLine() const {
	return m_errLine;
}