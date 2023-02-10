#pragma once
#include <vector>
#include <llvm\IR\BasicBlock.h>

struct Cycle {
	llvm::BasicBlock* start;
	llvm::BasicBlock* end;
};

class Cycles final {
public:
	void addCycle(const Cycle& info);
	void deleteCycle();

	Cycle& getCycleAtDepth(int depth);
	int getDepth() const;

private:
	std::vector<Cycle> m_cycles;
};

extern Cycles g_cycles;