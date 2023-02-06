#include "BasicType.h"

std::strong_ordering operator<=>(BasicType left, BasicType right) {
    return u8(left) <=> u8(right);
}

bool isNumeric(BasicType type) {
    return type >= BasicType::I8 && type <= BasicType::F64;
}

bool isInteger(BasicType type) {
    return type >= BasicType::I8 && type <= BasicType::U64;
}

bool isSigned(BasicType type) {
    return type >= BasicType::I8 && type <= BasicType::I64;
}

bool isUnsigned(BasicType type) {
    return type >= BasicType::U8 && type <= BasicType::U64;
}

bool isFloat(BasicType type) {
    return type >= BasicType::F32 && type <= BasicType::F64;
}

bool isChar(BasicType type) {
    return type >= BasicType::C8 && type <= BasicType::C32;
}

bool isString(BasicType type) {
    return type >= BasicType::STR8 && type <= BasicType::STR32;
}

bool isPrimitive(BasicType type) {
    return type <= BasicType::C32;
}

bool hasSubtypes(BasicType type) {
    return type > BasicType::STR32;
}

bool isUserDefined(BasicType type) {
    return type >= BasicType::CLASS;
}

bool isReference(BasicType type) {
    return type == BasicType::REFERENCE || type == BasicType::RVAL_REFERENCE;
}

bool isPointer(BasicType type) {
    return type >= BasicType::DYN_ARRAY && type <= BasicType::OPTIONAL;
}

int getBasicTypeSize(BasicType type) {
    static int TYPE_SIZE[] = {
        0, 8, 16, 32, 64, 8, 16, 32, 64,
        32, 64,
        1, 8, 16, 32,
        128, 128, 128,
        64, 128, 64, 64, -1, -1, -1, 64,
        -1, -1, -1, -1, -1
    };

    return TYPE_SIZE[u8(type)];
}
