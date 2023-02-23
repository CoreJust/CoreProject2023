#pragma once
#include <vector>

/*
*	Usage:
*		AggregatorIterator iter;
*		iter.addVector(-vec-);
*		...
*		
*		for (T* i = iter.begin(); i != nullptr; i = iter.next())...
*/

// Allows to iterate through several containers as if through one
template<typename T>
class AggregatorIterator final {
	using vectorT = std::vector<T>;

	std::vector<vectorT*> m_vecs;
	size_t m_pos = 0;
	size_t m_currentContainer = 0;

	T m_no_T;

public:
	void addVector(vectorT& vec) {
		if (vec.size()) {
			m_vecs.push_back(&vec);
		}
	}

	T& begin() {
		if (m_vecs.size() == 0) {
			return m_no_T;
		}

		return (*m_vecs[0])[0];
	}

	bool is_end() {
		return m_currentContainer >= m_vecs.size();
	}

	// Returns nullptr on the end
	T& next() {
		if (m_currentContainer >= m_vecs.size()) {
			return m_no_T;
		}

		m_pos++;
		while (m_pos >= m_vecs[m_currentContainer]->size()) {
			if (++m_currentContainer >= m_vecs.size()) {
				return m_no_T;
			}

			m_pos = 0;
		}

		return (*m_vecs[m_currentContainer])[m_pos];
	}
};
