#include "TypeNode.h"
#include <Module/LLVMGlobals.h>

std::vector<TypeNode> g_basicTypeNodes;

TypeNode::TypeNode(
    std::string name, 
    TypeQualities qualities, 
    std::shared_ptr<Type> type, 
    llvm::Type* llvmType,
    std::vector<Variable> fields, 
    std::vector<Function> methods, 
    std::vector<std::shared_ptr<TypeNode>> internalTypes
) : 
    name(std::move(name)), 
    qualities(qualities), 
    type(std::move(type)), 
    llvmType(llvmType),
    fields(std::move(fields)), 
    methods(std::move(methods)), 
    internalTypes(std::move(internalTypes)) {

}

TypeNode::TypeNode(TypeNode& other)
    : TypeNode(std::move(other)) {

}

TypeNode::TypeNode(TypeNode&& other) :
    name(std::move(other.name)), 
    qualities(other.qualities), 
    type(std::move(other.type)), 
    llvmType(other.llvmType),
    fields(std::move(other.fields)), 
    methods(std::move(other.methods)), 
    internalTypes(std::move(other.internalTypes)) {

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

bool TypeNode::isEquals(std::shared_ptr<TypeNode> other) {
    return name == other->name
        && qualities.getData() == other->qualities.getData()
        && type->equals(other->type)
        && fields.size() == other->fields.size()
        && methods.size() == other->methods.size()
        && internalTypes.size() == other->internalTypes.size();
}

SymbolType TypeNode::getSymbolType(const std::string& name, Visibility visibility, bool isStatic) const {
    for (auto& var : fields) {
        if (var.name == name && isAccessible(visibility, var.qualities.getVisibility())) {
            return SymbolType::VARIABLE;
        }
    }

    for (auto& fun : methods) {
        if (fun.prototype.getName() == name && isAccessible(visibility, fun.prototype.getQualities().getVisibility())) {
            return SymbolType::FUNCTION;
        }
    }

    for (auto& type : internalTypes) {
        if (type->name == name && isAccessible(visibility, type->qualities.getVisibility())) {
            return SymbolType::TYPE;
        }
    }

    return SymbolType::NO_SYMBOL;
}

Function* TypeNode::getMethod(const std::string& name, Visibility visibility, bool isStatic) {
    Function* result = nullptr;
    for (auto& fun : methods) {
        if (fun.prototype.getName() == name
            && isAccessible(visibility, fun.prototype.getQualities().getVisibility())
            && (fun.prototype.getQualities().getMethodType() == MethodType::STATIC) == isStatic) {
            if (result != nullptr) {
                return nullptr;
            }

            result = &fun;
        }
    }

    return result;
}

Function* TypeNode::getMethod(
    const std::string& name,
    const std::vector<std::shared_ptr<Type>>& argTypes,
    const std::vector<bool>& isCompileTime,
    bool isStatic
) {
    for (auto& fun : methods) {
        if (fun.prototype.getName() == name
            && (fun.prototype.getQualities().getMethodType() == MethodType::STATIC) == isStatic) {
            i32 score = fun.prototype.getSuitableness(argTypes, isCompileTime);
            if (score == 0) {
                return &fun;
            }
        }
    }

    return nullptr;
}

Function* TypeNode::chooseMethod(
    const std::string& name,
    const std::vector<std::shared_ptr<Type>>& argTypes,
    const std::vector<bool>& isCompileTime,
    Visibility visibility,
    bool isStatic
) {
    Function* result = nullptr;
    i32 bestScore = -1;
    for (auto& fun : methods) {
        if (fun.prototype.getName() == name
            && isAccessible(visibility, fun.prototype.getQualities().getVisibility())
            && (fun.prototype.getQualities().getMethodType() == MethodType::STATIC) == isStatic) {
            i32 score = fun.prototype.getSuitableness(argTypes, isCompileTime);
            if (score < 0) {
                continue;
            }

            if (result == nullptr || score < bestScore) {
                bestScore = score;
                result = &fun;

                if (score == 0) {
                    return result;
                }
            }
        }
    }

    return result;
}

Variable* TypeNode::getField(const std::string& name, Visibility visibility, bool isStatic) {
    for (auto& var : fields) {
        if (var.name == name
            && isAccessible(visibility, var.qualities.getVisibility())
            && (var.qualities.getVariableType() != VariableType::FIELD) == isStatic) {
            return &var;
        }
    }

    return nullptr;
}

std::shared_ptr<TypeNode> TypeNode::getType(const std::string& name, Visibility visibility) {
    for (auto& type : internalTypes) {
        if (type->name == name && isAccessible(visibility, type->qualities.getVisibility())) {
            return type;
        }
    }

    return nullptr;
}

std::shared_ptr<Type> TypeNode::genType(std::shared_ptr<TypeNode> typeNode, bool isConst) {
    if (typeNode->type && typeNode->type->basicType == BasicType::TYPE_NODE) {
        return typeNode->type;
    } else {
        return TypeNodeType::createType(std::move(typeNode), isConst);
    }
}

void initBasicTypeNodes() {
    g_basicTypeNodes.emplace_back(TypeNode{ "no_type", TypeQualities(), nullptr, llvm::Type::getVoidTy(g_context) });

    g_basicTypeNodes.emplace_back(TypeNode{ "i8", TypeQualities(), nullptr, llvm::Type::getInt8Ty(g_context) });
    g_basicTypeNodes.emplace_back(TypeNode{ "i16", TypeQualities(), nullptr, llvm::Type::getInt16Ty(g_context) });
    g_basicTypeNodes.emplace_back(TypeNode{ "i32", TypeQualities(), nullptr, llvm::Type::getInt32Ty(g_context) });
    g_basicTypeNodes.emplace_back(TypeNode{ "i64", TypeQualities(), nullptr, llvm::Type::getInt64Ty(g_context) });

    g_basicTypeNodes.emplace_back(TypeNode{ "u8", TypeQualities(), nullptr, llvm::Type::getInt8Ty(g_context) });
    g_basicTypeNodes.emplace_back(TypeNode{ "u16", TypeQualities(), nullptr, llvm::Type::getInt16Ty(g_context) });
    g_basicTypeNodes.emplace_back(TypeNode{ "u32", TypeQualities(), nullptr, llvm::Type::getInt32Ty(g_context) });
    g_basicTypeNodes.emplace_back(TypeNode{ "u64", TypeQualities(), nullptr, llvm::Type::getInt64Ty(g_context) });

    g_basicTypeNodes.emplace_back(TypeNode{ "f32", TypeQualities(), nullptr, llvm::Type::getFloatTy(g_context) });
    g_basicTypeNodes.emplace_back(TypeNode{ "f64", TypeQualities(), nullptr, llvm::Type::getDoubleTy(g_context) });

    g_basicTypeNodes.emplace_back(TypeNode{ "bool", TypeQualities(), nullptr, llvm::Type::getInt1Ty(g_context) });

    g_basicTypeNodes.emplace_back(TypeNode{ "c8", TypeQualities(), nullptr, llvm::Type::getInt8Ty(g_context) });
    g_basicTypeNodes.emplace_back(TypeNode{ "c16", TypeQualities(), nullptr, llvm::Type::getInt16Ty(g_context) });
    g_basicTypeNodes.emplace_back(TypeNode{ "c32", TypeQualities(), nullptr, llvm::Type::getInt32Ty(g_context) });

    g_basicTypeNodes.emplace_back(TypeNode{ "str8", TypeQualities(), nullptr, basicTypeToLLVM(BasicType::STR8) });
    g_basicTypeNodes.emplace_back(TypeNode{ "str16", TypeQualities(), nullptr, basicTypeToLLVM(BasicType::STR16) });
    g_basicTypeNodes.emplace_back(TypeNode{ "str32", TypeQualities(), nullptr, basicTypeToLLVM(BasicType::STR32) });


    TypeQualities qualities;
    qualities.setConst(true);
    for (size_t i = 0; i <= u8(BasicType::STR32); i++) {
        g_basicTypeNodes[i].qualities = qualities;
    }
}

TypeNode& getBasicTypeNode(BasicType type) {
    ASSERT(!hasSubtypes(type), "Complex types have no type node");
    return g_basicTypeNodes[u8(type)];
}
