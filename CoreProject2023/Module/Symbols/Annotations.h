#pragma once
#include <Utils/Defs.h>

enum class Visibility : u8 {
	LOCAL = 0,
	PRIVATE,
	DIRECT_IMPORT, // or ptotected for methods and fields
	PUBLIC
};

// Whether the symbol is visible with -from- access
bool isAccessible(Visibility from, Visibility symbol);

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

	virtual u64 getData() const;
};

/*
* bits starting from common qualities' last one:
*	4: is nomangling
*	5: is to add default imports
*/
class ModuleQualities final : public CommonQualities {
public:
	ModuleQualities();

	bool isManglingOn() const;
	void setMangling(bool isToMangle);

	bool isDefaultImports() const;
	void setDefaultImports(bool isToAdd);

	u64 getData() const override;
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
	TypeQualities();

	ClassType getClassType() const;
	void setClassType(ClassType type);

	bool isMoveOnly() const;
	void setMoveOnly(bool isMoveOnly);

	bool isConst() const;
	void setConst(bool isConst);

	u64 getData() const override;
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
*	6: is thread-local
*/
class VariableQualities final : public CommonQualities {
public:
	VariableQualities();

	VariableType getVariableType() const;
	void setVariableType(VariableType type);

	bool isThreadLocal() const;
	void setThreadLocal(bool isThreadLocal);

	u64 getData() const override;
};

enum class MethodType : u8 {
	COMMON = 0,
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

enum class FunctionKind : u8 {
	COMMON = 0,
	CONSTRUCTOR,
	DESTRUCTOR,
	OPERATOR
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
*	14: is noreturn
*	15: is noexcept
*	16-17: function kind
*/
class FunctionQualities final : public CommonQualities {
	u16 m_additionalData = 0;

public:
	FunctionQualities();

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

	bool isNoReturn() const;
	void setNoReturn(bool isNoReturn);

	bool isNoExcept() const;
	void setNoExcept(bool isNoExcept);

	FunctionKind getFunctionKind() const;
	void setFunctionKind(FunctionKind kind);

	u64 getData() const override;
};