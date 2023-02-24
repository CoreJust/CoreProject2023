#pragma once
#include <fstream>
#include <string>
#include <iostream>
#include <Utils/Defs.h>

// The file currently processed. Must be changed outside this module.
extern std::string g_currFileName;
extern std::string g_currFilePath; // format: *:/**/**/**.core

enum ErrorID : u32 {
	// Warnings
	W0001 = 0,

	// Errors
	// Lexer errors
	E1001_NO_SUCH_MODULE_FOUND, // Imported file is not found
	E1002_NO_ENDING_SEMICOLON, // import without ;
	E1003_MULTILINE_COMMENT_IS_NOT_CLOSED, // Multiline comment has ### in the beginning, but does not have ### in the end
	E1004_NO_CLOSING_BRACE, // A code block has opening brace ({), but no closing brace (}) is found

	E1051_UNKNOWN_ANNOTATION, // Unknown annotation
	E1052_WRONG_ANNOTATION, // Annotation before the element it could not annotate
	E1053_ANNOTATION_PARAMETER_UNSTATED, // Annotation's parameter unstated
	E1054_ANNOTATION_VALUE_UNSTATED, // Annotation's value unstated
	E1055_UNKNOWN_ANNOTATION_PARAMETER, // Annotation's parameter is unknown
	E1056_UNKNOWN_ANNOTATION_VALUE, // Annotation's value is unknown

	E1101_FLOAT_LITERAL_IMPROPER_POSTFIX, // Literal 0.smth does not end with f32/f64 nor with normal number
	E1102_NUMBER_LITERAL_IMPROPER_POSTFIX, // Literal xxxxx does not end with i/u/c/f nor normal number
	E1103_STRING_IMPROPER_POSTFIX, // Literal "..." does not end with str8/16/32 nor nothing
	E1104_CHARACTER_IMPROPER_POSTFIX, // Literal '..' does not end with c8/16/32 nor nothing
	E1105_NUMBER_HAS_UNKNOWN_NUMERIC_SYSTEM, // Number starts with 0..xxxx, where .. is not x, b, o
	E1106_NUMBER_DELIMITER_FORBIDDEN, // Number has delimiter (') where it is banned
	E1107_INCONSISTENT_HEX_NUMBER, // Hexadecimal number has both cases (upper and lower)
	E1108_STRING_CONTAINS_CHARS_GREATER_THAN_FORMAT_ALLOWS, // e.g. ascii string has character with code 1024

	E1111_NO_CLOSING_APOSTROPHE, // Character literal has no closing '
	E1112_NO_CLOSING_QUOTE, // String literal has no closing " or """


	// Parser errors
	E2001_EXPRESSION_NOT_FOUND, // No expression where it is expected
	E2002_UNEXPECTED_TOKEN, // Expected token not met
	E2003_UNKNOWN_IDENTIFIER, // No such variable/function/type
	E2004_MANY_FUNCTIONS_WITH_SUCH_NAME, // Many functions found while searching by name
	E2005_NO_SUITABLE_FUNCTION, // No function can be called with such argument types
	E2006_NO_SUCH_MEMBER, // The type has no member with such name
	E2007_MEMBER_IS_INACCESSIBLE, // The member's visibility makes the member inaccessible here
	E2008_INCORRECT_ARRAY_ELEMENT_ACCESS, // Incorrect expression of getting an array element by index (arr[idx])
	E2009_TOO_MANY_ARRAY_ELEMENTS, // Array value was declared as -type-[size] { ... } and there are more elements than size

	E2101_VAR_HAS_NO_INIT, // Variable declared through var cannot deduce type without initial value
	E2102_CANNOT_BE_CALLED, // Expression cannot be called through -expr-(...)
	E2103_NOT_A_REFERENCE, // Expression expected to be of reference
	E2104_FUNCTION_BODY_MISMATCHED, // Function has no body
	E2105_VA_ARGS_MUST_BE_THE_LAST_ARGUMENT, // There are arguments coming after va_args (...)
	E2106_THIS_USED_OUTSIDE_TYPE, // This is used not in a method/constructor/destructor
	E2107_TYPE_CONSTRUCTOR_IS_STATIC, // A static constructor/type conversion in a class(or struct/...)
	E2108_OPERATOR_IMPOSSIBLE_ARGUMENTS_NUMBER, // An impossible number of arguments of an operator
	E2109_CONDITIONAL_OPERATOR_MUST_RETURN_BOOL, // A conditional user-defined operator must be of bool type

	
	// Type errors
	E3001_TYPE_NOT_SPECIFIED, // No type expression found where it is expected
	E3002_UNEXPECTED_TOKEN_WHILE_PARSING_TYPE, // Expected token not met
	E3003_NOT_A_TYPE, // The identifier is not a type (e.g. variable or function)

	E3051_REFERENCE_TO_REFERENCE, // Tried to create a reference to a reference (like i32 & &&)
	E3052_NEGATIVE_SIZE_ARRAY, // Tried to create an array of negative or zero size
	E3053_ONLY_POINTERS_CAN_BE_DEREFERENCED, // Tried to dereference (*a) not a pointer-type value
	E3054_CANNOT_NEGATE_UNSIGNED_INT, // Tried to get a negative of unsigned int (-(uint<>))
	E3055_CANNOT_GET_REFERENCE, // Tried to get a reference from a non-reference type (e.g. ref 5)
	E3056_MUST_BE_A_REFERENCE, // A reference-type value expected
	E3057_IS_A_CONSTANT, // A constant met where a mutable value was expected

	E3101_CANNOT_BE_IMPLICITLY_CONVERTED, // Imposible implicit conversion of types
	E3102_CANNOT_BE_EXPLICITLY_CONVERTED, // Imposible explicit conversion of types
	E3103_CANNOT_CONVERT_TO_ONE, // Imposible to convert types to one
	E3104_NO_SUITABLE_CONSTRUCTOR, // Constructor suitable for certain arguments not found


	// Internal errors
	E4001_WRONGLY_READ_STRING_BAD_SIZE, // Cannot convert string from utf32 if its size is not multiple of 4

	E4051_LOADING_MODULE_SYMBOLS_TWICE, // The module symbols were added to symbol table twice
	E4052_NO_MODULE_FOUND_BY_ALIAS, // Tried to get symbol of module with a wrong alias
	E4053_CANNOT_SET_NO_MODULE_AS_CURRENT, // Tried to set g_module to nullptr (since no module was found)
};

std::string errorIDToString(ErrorID id);

namespace ErrorManager {
	enum OutputMode : u8 {
		NO_MODE = 0,
		CONSOLE = 0b1,
		LOGS = 0b10
	};

	extern std::ofstream* _file;
	extern OutputMode _mode;

	// Should be called before using ErrorManager!
	void init(OutputMode mode, const std::string& log_file = "");

	/// Resets the state of ErrorManager. Should be called only after init!
	void clear();

	void lexerError(ErrorID id, int line, const std::string& data);
	void parserError(ErrorID id, int line, const std::string& data);
	void typeError(ErrorID id, int line, const std::string& data);
	void internalError(ErrorID id, int line, const std::string& data);

	void warning(ErrorID id, const std::string& data);

	void printFileLine(int line);
};