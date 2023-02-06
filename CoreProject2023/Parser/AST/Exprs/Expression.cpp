#include "Expression.h"

const std::unique_ptr<Type>& Expression::getType() const {
    return m_type;
}
