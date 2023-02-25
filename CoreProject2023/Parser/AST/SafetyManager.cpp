#include "SafetyManager.h"
#include <Utils/ErrorManager.h>

SafetyManager g_safety;

void SafetyManager::push(Safety safety) {
	m_safeties.push_back(safety);
}

void SafetyManager::pop() {
	m_safeties.pop_back();
}

Safety SafetyManager::getCurrentSafety() const {
	if (!m_safeties.size()) {
		return Safety::SAFE_ONLY;
	}

	return m_safeties.back();
}

bool SafetyManager::isPossible(Safety safety) const {
	if (getCurrentSafety() == Safety::SAFE_ONLY) {
		return safety != Safety::UNSAFE;
	}

	return true;
}

void SafetyManager::tryUse(Safety safety, i64 errLine) const {
	tryUse(getCurrentSafety(), safety, errLine);
}

void SafetyManager::tryUse(Safety currentSafety, Safety safety, i64 errLine) const {
	if (currentSafety == Safety::SAFE_ONLY && safety == Safety::UNSAFE) {
		ErrorManager::parserError(
			ErrorID::E2201_UNSAFE_CODE_IN_SAFE_ONLY,
			errLine,
			""
		);
	}
}
