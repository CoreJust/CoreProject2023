#include "String.h"

std::vector<std::string> split(const std::string& str, char delimiter) {
	std::vector<std::string> result;
	result.push_back("");

	for (size_t i = 0; i < str.size(); i++) {
		if (str[i] != delimiter) {
			result.back() += str[i];
		} else {
			result.push_back("");
			do {
				i++;
			} while (i < str.size() && str[i] == delimiter);
			i--;
		}
	}

	if (result.back().size() == 0) {
		result.pop_back();
	}

	return result;
}

void replaceChar(std::string& str, char old, char newChar) {
	for (auto& ch : str) {
		if (ch == old) {
			ch = newChar;
		}
	}
}


int charToDecimal(char c) {
	return isdigit(c) ?
		c - '0'
		: tolower(c) - 'a' + 10;
}

double to_double(const std::string& s, int num_sys) {
	double r = 0.0;
	bool wasPoint = false;
	bool isNeg = false;
	double afterPoint = 1;
	size_t i = 0;

	if (s[0] == '-') {
		isNeg = ++i;
	}

	for (; i < s.size(); ++i) {
		if (s[i] == '.') {
			wasPoint = true;
		} else if (!wasPoint) {
			r *= num_sys;
			r += charToDecimal(s[i]);
		} else {
			afterPoint *= num_sys;
			r += charToDecimal(s[i]) / afterPoint;
		}
	}

	return isNeg ? -r : r;
}

u64 to_u64(const std::string& s, int num_sys) {
	u64 r = 0;
	for (size_t i = 0; i < s.size(); ++i) {
		r *= num_sys;
		r += charToDecimal(s[i]);
	}

	return r;
}

void toDecimal(std::string& s, int nsys) {
	if (nsys == 10)
		return;

	if (s.find('.') != std::string::npos) {
		s = std::to_string(to_double(s, nsys));
	} else {
		s = std::to_string(to_u64(s, nsys));
	}
}

void addUtf32(std::string& to, u32 ch) {
	to += "    ";
	*(u32*)(&to.back() - 3) = ch;
}

u32 utf32ToUtf16(std::string& from) {
	if (from.size() % 4 != 0)
		return 1; // wrong string format

	int j = 0;
	for (int i = 0; i < from.size(); i += 4) {
		if (*(u32*)(&from[i]) > 0xffff) {
			return *(u32*)(&from[i]); // non-translatable character
		}

		*(u16*)(&from[j]) = u16(*(u32*)&from[i]);
		j += 2;
	}

	from.resize(j);
	return 0;
}

u32 utf32ToASCII(std::string& from) {
	if (from.size() % 4 != 0)
		return 1; // wrong string format

	int j = 0;
	for (int i = 0; i < from.size(); i += 4) {
		if (*(u32*)(&from[i]) > 0xff) {
			return *(u32*)(&from[i]); // non-translatable character
		}

		from[j++] = u8(*(u32*)&from[i]);
	}

	from.resize(j);
	return 0;
}