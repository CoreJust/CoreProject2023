#include "Annotations.h"

Visibility CommonQualities::getVisibility() const {
    return Visibility(m_data & 3);
}

void CommonQualities::setVisibility(Visibility visibility) {
    m_data = (m_data & ~3) | (u8)visibility;
}

Safety CommonQualities::getSafety() const {
    return Safety((m_data >> 2) & 3);
}

void CommonQualities::setSafety(Safety visibility) {
    m_data = (m_data & ~12) | ((u8)visibility << 2);
}

ModuleQualities::ModuleQualities() {
    setVisibility(Visibility::PUBLIC);
    setSafety(Safety::SAFE_ONLY);
    setMangling(true);
}

bool ModuleQualities::isManglingOn() const {
    return bool(m_data & (1 << 4));
}

void ModuleQualities::setMangling(bool isToMangle) {
    m_data = (m_data & ~0b10000) | (u8(isToMangle ? 1 : 0) << 4);
}

TypeQualities::TypeQualities() {
    setVisibility(Visibility::PUBLIC);
    setSafety(Safety::SAFE_ONLY);
    setClassType(ClassType::COMMON);
    setConst(false);
    setMoveOnly(false);
}

ClassType TypeQualities::getClassType() const {
    return ClassType((m_data >> 4) & 3);
}

void TypeQualities::setClassType(ClassType type) {
    m_data = (m_data & ~0b110000) | ((u8)type << 4);
}

bool TypeQualities::isMoveOnly() const {
    return bool(m_data & (1 << 6));
}

void TypeQualities::setMoveOnly(bool isMoveOnly) {
    m_data = (m_data & ~0b1000000) | (u8(isMoveOnly ? 1 : 0) << 6);
}

bool TypeQualities::isConst() const {
    return bool(m_data & (1 << 7));
}

void TypeQualities::setConst(bool isConst) {
    m_data = (m_data & ~0b10000000) | (u8(isConst ? 1 : 0) << 7);
}

VariableQualities::VariableQualities() {
    setVisibility(Visibility::PUBLIC);
    setSafety(Safety::SAFE_ONLY);
    setVariableType(VariableType::COMMON);
}

VariableType VariableQualities::getVariableType() const {
    return VariableType((m_data >> 4) & 3);
}

void VariableQualities::setVariableType(VariableType type) {
    m_data = (m_data & ~0b110000) | ((u8)type << 4);
}

bool VariableQualities::isThreadLocal() const {
    return bool(m_data & (1 << 6));
}

void VariableQualities::setThreadLocal(bool isThreadLocal) {
    m_data = (m_data & ~0b1000000) | (u8(isThreadLocal ? 1 : 0) << 6);
}

FunctionQualities::FunctionQualities() {
    setFunctionKind(FunctionKind::COMMON);
    setMethodType(MethodType::COMMON);
    setVisibility(Visibility::PUBLIC);
    setSafety(Safety::SAFE_ONLY);
    setIsMethod(false);
    setCallingConvention(CallingConvention::CCALL);
    setMangling(true);
    setNoReturn(false);
    setNoExcept(false);
}

bool FunctionQualities::isMethod() const {
    return bool((m_data >> 4) & 1);
}

void FunctionQualities::setIsMethod(bool isMethod) {
    m_data = (m_data & ~0b10000) | ((isMethod ? 1 : 0) << 4);
}

bool FunctionQualities::isNative() const {
    return m_data & 32;
}

void FunctionQualities::setNative(bool isNative) {
    m_data = (m_data & ~0b100000) | (isNative << 5);
}

MethodType FunctionQualities::getMethodType() const {
    return MethodType((m_data >> 6) & 3);
}

void FunctionQualities::setMethodType(MethodType type) {
    m_data = (m_data & ~0b11000000) | ((u8)type << 6);
}

bool FunctionQualities::isOverride() const {
    return m_additionalData & 1;
}

void FunctionQualities::setOverride(bool isOverride) {
    m_additionalData = (m_additionalData & ~1) | isOverride;
}

bool FunctionQualities::isImplicit() const {
    return m_additionalData & 2;
}

void FunctionQualities::setImplicit(bool isImplicit) {
    m_additionalData = (m_additionalData & ~3) | (isImplicit << 1);
}

CallingConvention FunctionQualities::getCallingConvention() const {
    return CallingConvention((m_additionalData >> 2) & 7);
}

void FunctionQualities::setCallingConvention(CallingConvention convention) {
    m_additionalData = (m_additionalData & ~0b11100) | ((u8)convention << 2);
}

bool FunctionQualities::isManglingOn() const {
    return bool((m_additionalData >> 5) & 1);
}

void FunctionQualities::setMangling(bool isToMangle) {
    m_additionalData = (m_additionalData & ~0b100000) | ((isToMangle ? 1 : 0) << 5);
}

bool FunctionQualities::isNoReturn() const {
    return bool((m_additionalData >> 6) & 1);
}

void FunctionQualities::setNoReturn(bool isNoReturn) {
    m_additionalData = (m_additionalData & ~0b1000000) | ((isNoReturn ? 1 : 0) << 6);
}

bool FunctionQualities::isNoExcept() const {
    return bool((m_additionalData >> 7) & 1);
}

void FunctionQualities::setNoExcept(bool isNoExcept) {
    m_additionalData = (m_additionalData & ~0b10000000) | ((isNoExcept ? 1 : 0) << 7);
}

FunctionKind FunctionQualities::getFunctionKind() const {
    return FunctionKind((m_additionalData >> 8) & 3);
}

void FunctionQualities::setFunctionKind(FunctionKind kind) {
    m_additionalData = (m_additionalData & ~0b1100000000) | ((u8)kind << 8);
}
