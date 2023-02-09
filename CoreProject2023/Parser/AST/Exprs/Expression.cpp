#include "Expression.h"

const std::unique_ptr<Type>& Expression::getType() const {
    return m_type;
}

bool Expression::isCompileTime() const {
    return false;
}

bool Expression::isRVal() const {
    return m_isRVal;
}
