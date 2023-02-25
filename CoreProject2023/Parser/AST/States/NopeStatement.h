#pragma once
#include "Statement.h"

// Empty statement (e.g. ;)
class NopeStatement final : public Statement {
	FRIEND_CLASS_VISITORS

public:
	NopeStatement();

	void accept(Visitor* visitor, std::unique_ptr<Statement>& node) override;
	void generate() override;
};