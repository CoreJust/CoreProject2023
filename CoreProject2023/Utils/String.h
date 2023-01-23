#pragma once
#include <string>
#include <vector>
#include "Defs.h"

std::vector<std::string> split(const std::string& str, char delimiter);

// Replaces every -old- with -newChar-
void replaceChar(std::string& str, char old, char newChar);


int charToDecimal(char c);
double to_double(const std::string& s, int num_sys);
u64 to_u64(const std::string& s, int num_sys);

// converts the number in string from nsys numeric system (e.g. "ff") to decimal ("255")
void toDecimal(std::string& s, int nsys);

// adds four bytes to string to represent utf-32
void addUtf32(std::string& to, u32 ch);

// conversion functions return 0 if no errors occured, 1 if wrong string length, else - wrong character
// converts utf-32 string to utf-16 string, changes the original string
u32 utf32ToUtf16(std::string& from);

// converts utf-32 string to ASCII string, changes the original string
u32 utf32ToASCII(std::string& from);