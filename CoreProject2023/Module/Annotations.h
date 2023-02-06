#pragma once
#include <Utils/Defs.h>

enum class Visibility : u8 {
	LOCAL = 0,
	PRIVATE,
	PUBLIC,
	DIRECT_IMPORT
};

enum class Safety : u8 {
	UNSAFE = 0,
	SAFE_ONLY, // default state, unsafe constructs are not allowed
	SAFE
};

/*
* bits:
*	0-1: visibility
*	2-3: safety
*/
class CommonQualities {
protected:
	u8 m_data = 0;

public:
	Visibility getVisibility() const;
	void setVisibility(Visibility visibility);

	Safety getSafety() const;
	void setSafety(Safety visibility);
};

enum class ProgramType : u8 {
	PROGRAM = 0,
	OBJECT,
	DYNAMIC_LIBRARY,
	STATIC_LIBRARY
};

/*
* bits starting from common qualities' last one:
*	4-5: program type
*/
class ModuleQualities final : public CommonQualities {
public:
	ProgramType getProgramType() const;
	void setProgramType(ProgramType type);
};

enum class ClassType : u8 {
	COMMON = 0,
	ABSTRACT,
	FINAL
};

/*
* bits starting from common qualities' last one:
*	4-5: class type
*	6: is move only
*	7: is constant
*/
class TypeQualities final : public CommonQualities {
public:
	ClassType getClassType() const;
	void setClassType(ClassType type);

	bool isMoveOnly() const;
	void setMoveOnly(bool isMoveOnly);

	bool isConst() const;
	void setConst(bool isConst);
};

enum class VariableType : u8 {
	COMMON = 0,
	CONST,
	EXTERN,
	FIELD // of a user defined type
};

/*
* bits starting from common qualities' last one:
*	4-5: variable type
*/
class VariableQualities final : public CommonQualities {
public:
	VariableType getVariableType() const;
	void setVariableType(VariableType type);
};

enum class MethodType : u8 {
	COMMOM = 0,
	STATIC,
	VIRTUAL,
	ABSTRACT
};

enum class CallingConvention : u8 {
	CCALL = 0,
	STDCALL,
	FASTCALL,
	THISCALL,
	VECTORCALL,
	COLDCALL,
	TAILCALL
};

/*
* bits starting from common qualities' last one:
*	4: isMethod
*	5: is native
*	6-7: method type (if method)
*	8: is override (if method)
*	9: is explicit(0) or implicit(1) (if type cast function)
*	10-12: calling convention (default ccall)
*	13: is mangling on
*/
class FunctionQualities final : public CommonQualities {
	u8 m_additionalData = 0;

public:
	bool isMethod() const;
	void setIsMethod(bool isMethod);

	bool isNative() const;
	void setNative(bool isNative);

	MethodType getMethodType() const;
	void setMethodType(MethodType type);

	bool isOverride() const;
	void setOverride(bool isOverride);

	bool isImplicit() const;
	void setImplicit(bool isImplicit);

	CallingConvention getCallingConvention() const;
	void setCallingConvention(CallingConvention convention);

	bool isManglingOn() const;
	void setMangling(bool isToMangle);
};