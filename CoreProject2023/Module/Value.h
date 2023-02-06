#pragma once
#include <vector>
#include "BasicType.h"

constexpr size_t VALUE_UNION_DATA_SIZE = sizeof(std::string) / sizeof(u64);

// Not to be used separately
union _ValueUnion {
	u64 uintVal; // or chars/bools
	i64 intVal;
	f64 floatVal;
	std::string strVal;
	u64 _data[VALUE_UNION_DATA_SIZE];

	_ValueUnion();
	_ValueUnion(u64 val);
	_ValueUnion(i64 val);
	_ValueUnion(f64 val);
	_ValueUnion(std::string val);
	_ValueUnion(_ValueUnion& other);
	_ValueUnion(_ValueUnion&& other);
	~_ValueUnion();
};

// In case of pointer it is always nullptr
struct Value {
	BasicType type;
	_ValueUnion value;

	Value(BasicType type, _ValueUnion val);
	Value(Value&) = default;
	Value(Value&&) = default;
	~Value() = default;
};

struct ArrayValue {
	BasicType type;
	std::vector<_ValueUnion> values;

	ArrayValue(BasicType type, std::vector<_ValueUnion> values);
	~ArrayValue() = default;
};

struct TupleValue {
	std::vector<Value> values;

	TupleValue(std::vector<Value> values);
	~TupleValue() = default;
};
