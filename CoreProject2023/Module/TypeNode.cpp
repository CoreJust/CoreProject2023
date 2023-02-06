#include "TypeNode.h"
#include <Parser/AST/INode.h>

std::vector<TypeNode> g_basicTypeNodes;

TypeNode::TypeNode(std::string name, TypeQualities qualities, std::unique_ptr<Type> type, llvm::Type* llvmType,
    std::vector<Variable> fields, std::vector<Function> methods, std::vector<std::unique_ptr<TypeNode>> internalTypes)
    : name(std::move(name)), qualities(qualities), type(std::move(type)), llvmType(llvmType),
    fields(std::move(fields)), methods(std::move(methods)), internalTypes(std::move(internalTypes)) {

}

TypeNode::TypeNode(TypeNode& other)
    : TypeNode(std::move(other)) {

}

TypeNode::TypeNode(TypeNode&& other) 
    : name(std::move(other.name)), qualities(other.qualities), type(std::move(other.type)), llvmType(other.llvmType),
    fields(std::move(other.fields)), methods(std::move(other.methods)), internalTypes(std::move(other.internalTypes)) {

}

TypeNode& TypeNode::operator=(TypeNode&& other) {
    name = std::move(other.name);
    qualities = other.qualities;
    type = std::move(other.type);
    llvmType = other.llvmType;
    fields = std::move(other.fields);
    methods = std::move(other.methods);
    internalTypes = std::move(other.internalTypes);
    return *this;
}

void initBasicTypeNodes() {
    g_basicTypeNodes.emplace_back(TypeNode{ "no_type", TypeQualities(), nullptr, llvm::Type::getVoidTy(g_context), {}, {}, {} });

    g_basicTypeNodes.emplace_back(TypeNode{ "i8", TypeQualities(), nullptr, llvm::Type::getInt8Ty(g_context), {}, {}, {} });
    g_basicTypeNodes.emplace_back(TypeNode{ "i16", TypeQualities(), nullptr, llvm::Type::getInt16Ty(g_context), {}, {}, {} });
    g_basicTypeNodes.emplace_back(TypeNode{ "i32", TypeQualities(), nullptr, llvm::Type::getInt32Ty(g_context), {}, {}, {} });
    g_basicTypeNodes.emplace_back(TypeNode{ "i64", TypeQualities(), nullptr, llvm::Type::getInt64Ty(g_context), {}, {}, {} });

    g_basicTypeNodes.emplace_back(TypeNode{ "u8", TypeQualities(), nullptr, llvm::Type::getInt8Ty(g_context), {}, {}, {} });
    g_basicTypeNodes.emplace_back(TypeNode{ "u16", TypeQualities(), nullptr, llvm::Type::getInt16Ty(g_context), {}, {}, {} });
    g_basicTypeNodes.emplace_back(TypeNode{ "u32", TypeQualities(), nullptr, llvm::Type::getInt32Ty(g_context), {}, {}, {} });
    g_basicTypeNodes.emplace_back(TypeNode{ "u64", TypeQualities(), nullptr, llvm::Type::getInt64Ty(g_context), {}, {}, {} });

    g_basicTypeNodes.emplace_back(TypeNode{ "f32", TypeQualities(), nullptr, llvm::Type::getFloatTy(g_context), {}, {}, {} });
    g_basicTypeNodes.emplace_back(TypeNode{ "f64", TypeQualities(), nullptr, llvm::Type::getDoubleTy(g_context), {}, {}, {} });

    g_basicTypeNodes.emplace_back(TypeNode{ "bool", TypeQualities(), nullptr, llvm::Type::getInt1Ty(g_context), {}, {}, {} });

    g_basicTypeNodes.emplace_back(TypeNode{ "c8", TypeQualities(), nullptr, llvm::Type::getInt8Ty(g_context), {}, {}, {} });
    g_basicTypeNodes.emplace_back(TypeNode{ "c16", TypeQualities(), nullptr, llvm::Type::getInt16Ty(g_context), {}, {}, {} });
    g_basicTypeNodes.emplace_back(TypeNode{ "c32", TypeQualities(), nullptr, llvm::Type::getInt32Ty(g_context), {}, {}, {} });

    g_basicTypeNodes.emplace_back(TypeNode{ "str8", TypeQualities(), nullptr, 
        llvm::StructType::get(g_context, { llvm::Type::getInt8PtrTy(g_context), llvm::Type::getInt64Ty(g_context) }, true),
        {}, {}, {}});

    g_basicTypeNodes.emplace_back(TypeNode{ "str16", TypeQualities(), nullptr,
        llvm::StructType::get(g_context, { llvm::Type::getInt16PtrTy(g_context), llvm::Type::getInt64Ty(g_context) }, true),
        {}, {}, {} });

    g_basicTypeNodes.emplace_back(TypeNode{ "str32", TypeQualities(), nullptr,
        llvm::StructType::get(g_context, { llvm::Type::getInt32PtrTy(g_context), llvm::Type::getInt64Ty(g_context) }, true),
        {}, {}, {} });


    TypeQualities qualities;
    qualities.setConst(true);
    qualities.setMoveOnly(false);
    qualities.setSafety(Safety::SAFE);
    qualities.setVisibility(Visibility::PUBLIC);

    for (size_t i = 0; i <= u8(BasicType::STR32); i++) {
        g_basicTypeNodes[i].qualities = qualities;
    }
}

TypeNode& getBasicTypeNode(BasicType type) {
    ASSERT(!hasSubtypes(type) && type != BasicType::NO_TYPE, "Complex types have no type node");
    return g_basicTypeNodes[u8(type)];
}
