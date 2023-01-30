#pragma once
#include "Statement.h"

class BlockStatement final : public Statement {
	FRIEND_CLASS_VISITORS

public:
	BlockStatement(std::vector<std::unique_ptr<Statement>> states);

	void accept(Visitor* visitor, std::unique_ptr<Statement>& node) override;
	void generate() override;

private:
	std::vector<std::unique_ptr<Statement>> m_states;
};