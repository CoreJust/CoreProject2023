#pragma once
#include <vector>
#include <Module/Symbols/Annotations.h>

class SafetyManager final {
private:
	std::vector<Safety> m_safeties;

public:
	void push(Safety safety);
	void pop();

	Safety getCurrentSafety() const;
	bool isPossible(Safety safety) const;

	void tryUse(Safety safety, i64 errLine) const;
	void tryUse(Safety currentSafety, Safety safety, i64 errLine) const;
};

extern SafetyManager g_safety;