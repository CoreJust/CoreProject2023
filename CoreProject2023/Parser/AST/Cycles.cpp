#include "Cycles.h"

Cycles g_cycles;

void Cycles::addCycle(const Cycle& info) {
	m_cycles.push_back(info);
}

void Cycles::deleteCycle() {
	m_cycles.pop_back();
}

Cycle& Cycles::getCycleAtDepth(int depth) {
	return m_cycles[m_cycles.size() - depth - 1];
}

int Cycles::getDepth() const {
	return m_cycles.size();
}
