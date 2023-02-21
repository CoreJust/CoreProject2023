#include "Expression.h"

const std::unique_ptr<Type>& Expression::getType() const {
    return m_type;
}

bool Expression::isCompileTime() const {
    return false;
}

bool Expression::isLVal() const {
    return isReference(m_type->basicType);
}
