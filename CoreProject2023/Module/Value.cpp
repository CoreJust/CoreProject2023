#include "Value.h"

_ValueUnion::_ValueUnion() {
	memset(_data, 0, sizeof(std::string));
}

_ValueUnion::_ValueUnion(u64 val)
	: uintVal(val) {

}

_ValueUnion::_ValueUnion(i64 val)
	: intVal(val) {

}

_ValueUnion::_ValueUnion(f64 val)
	: floatVal(val) {

}

_ValueUnion::_ValueUnion(std::string val)
	: strVal(val) {

}

_ValueUnion::_ValueUnion(_ValueUnion& other) {
	for (size_t i = 0; i < VALUE_UNION_DATA_SIZE; i++) {
		_data[i] = 0;
		if (other._data[i]) {
			strVal = other.strVal;
			return;
		}
	}

	_data[0] = other._data[0];
}

_ValueUnion::_ValueUnion(_ValueUnion&& other) {
	memcpy(_data, other._data, VALUE_UNION_DATA_SIZE * sizeof(u64));
	memset(other._data, 0, VALUE_UNION_DATA_SIZE * sizeof(u64));
}

_ValueUnion::~_ValueUnion() {
	for (size_t i = 0; i < VALUE_UNION_DATA_SIZE; i++) {
		if (_data[i]) {
			strVal.~basic_string();
			return;
		}
	}
}

Value::Value(BasicType type, _ValueUnion val)
	: type(type), value(std::move(val)) {

}

ArrayValue::ArrayValue(BasicType type, std::vector<_ValueUnion> values)
	: type(type), values(std::move(values)) {

}

TupleValue::TupleValue(std::vector<Value> values) 
	: values(std::move(values)) {

}
