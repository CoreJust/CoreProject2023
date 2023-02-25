#pragma once
#include <memory>
#include <Utils/Defs.h>
#include "SafetyManager.h"

#define FRIEND_CLASS_VISITORS \
	friend class Visitor;

class Visitor;

class INode {
public:
	INode();

	virtual std::string toString() const = 0;

	u64 getErrLine() const;
	Safety getSafety() const;

protected:
	u64 m_errLine;
	Safety m_safety;

protected:
	static std::string s_tabs; // for conversion to string
};

struct TerminatorAdded {

};
