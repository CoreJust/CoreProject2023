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
	E1108_STRING_CONTAINS_CHARS_BIGGER_THAN_FORMAT_ALLOWS, // e.g. ascii string has character with code 1024

	E1111_NO_CLOSING_APOSTROPHE, // Character literal has no closing '


	// Parser errors
	E2001_EXPRESSION_NOT_FOUND, // No expression where it is expected
	E2002_UNEXPECTED_TOKEN, // Expected token not met

	
	// Type errors
	E3001,


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
};