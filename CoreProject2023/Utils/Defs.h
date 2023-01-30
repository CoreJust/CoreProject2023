#pragma once
#include <cstdint>
#include <iostream>

#ifdef _DEBUG
#define ASSERT(expr, message)\
	if (!(expr)) {\
		std::cout << __FILE__ << ": assertion failed on line " << __LINE__ << " : " << (message) << std::endl;\
	}
#else
#define ASSERT(expr, message)
#endif

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using u1 = bool;
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using c8 = char8_t;
using c16 = char16_t;
using c32 = char32_t;

using f32 = float;
using f64 = double;