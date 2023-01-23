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

ProgramType ModuleQualities::getProgramType() const {
    return ProgramType((m_data >> 4) & 3);
}

void ModuleQualities::setProgramType(ProgramType type) {
    m_data = (m_data & ~0b110000) | ((u8)type << 4);
}

ClassType TypeQualities::getClassType() const {
    return ClassType((m_data >> 4) & 3);
}

void TypeQualities::setClassType(ClassType type) {
    m_data = (m_data & ~0b110000) | ((u8)type << 4);
}

VariableType VariableQualities::getVariableType() const {
    return VariableType((m_data >> 4) & 3);
}

void VariableQualities::setVariableType(VariableType type) {
    m_data = (m_data & ~0b110000) | ((u8)type << 4);
}

FunctionType FunctionQualities::getFunctionType() const {
    return FunctionType((m_data >> 4) & 1);
}

void FunctionQualities::setFunctionType(FunctionType type) {
    m_data = (m_data & ~0b10000) | ((u8)type << 4);
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