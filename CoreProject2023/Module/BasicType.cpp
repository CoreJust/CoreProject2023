#include "BasicType.h"
#include "LLVMGlobals.h"

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

bool isTruePointer(BasicType type) {
    return type == BasicType::ARRAY || type == BasicType::POINTER || type == BasicType::FUNCTION;
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

llvm::Type* basicTypeToLLVM(BasicType type) {
    switch (type) {
        case BasicType::NO_TYPE: return llvm::Type::getVoidTy(g_context);
        case BasicType::I8: return llvm::Type::getInt8Ty(g_context);
        case BasicType::I16: return llvm::Type::getInt16Ty(g_context);
        case BasicType::I32: return llvm::Type::getInt32Ty(g_context);
        case BasicType::I64: return llvm::Type::getInt64Ty(g_context);
        case BasicType::U8: return llvm::Type::getInt8Ty(g_context);
        case BasicType::U16: return llvm::Type::getInt16Ty(g_context);
        case BasicType::U32: return llvm::Type::getInt32Ty(g_context);
        case BasicType::U64: return llvm::Type::getInt64Ty(g_context);
        case BasicType::F32: return llvm::Type::getFloatTy(g_context);
        case BasicType::F64: return llvm::Type::getDoubleTy(g_context);
        case BasicType::BOOL: return llvm::Type::getInt1Ty(g_context);
        case BasicType::C8: return llvm::Type::getInt8Ty(g_context);
        case BasicType::C16: return llvm::Type::getInt16Ty(g_context);
        case BasicType::C32: return llvm::Type::getInt32Ty(g_context);
        case BasicType::STR8:
            return llvm::StructType::get(
                g_context, 
                { 
                    llvm::Type::getInt8PtrTy(g_context), // .data
                    llvm::Type::getInt64Ty(g_context) // .size
                }, 
                true // is packed
            );
        case BasicType::STR16:
            return llvm::StructType::get(
                g_context,
                { 
                    llvm::Type::getInt16PtrTy(g_context), // .data
                    llvm::Type::getInt64Ty(g_context) // .size
                }, 
                true // is packed
            );
        case BasicType::STR32:
            return llvm::StructType::get(
                g_context,
                { 
                    llvm::Type::getInt32PtrTy(g_context), // .data
                    llvm::Type::getInt64Ty(g_context) // .size
                }, 
                true // is packed
            );
    default: return nullptr;
    }
}
