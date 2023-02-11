#pragma once
#include <memory>
#include <Utils/Defs.h>

#define FRIEND_CLASS_VISITORS \
	friend class Visitor;

class Visitor;

class INode {
public:
	INode();

	u64 getErrLine() const;

protected:
	u64 m_errLine;
};

struct TerminatorAdded {

};
