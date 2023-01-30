#include "ErrorManager.h"

std::string g_currFileName = "";
std::string g_currFilePath = "";

std::ofstream* ErrorManager::_file;
ErrorManager::OutputMode ErrorManager::_mode;

std::string ErrorIDStrings[] = {
	"W0001: ",

	"E1001: No such module found",
	"E1002: Statement has no ending semicolon (;)",
	"E1003: Multiline comment has no ending (###)",
	"E1004: No closing brace ({) found",
	"E1051: Unknown annotation",
	"E1052: Wrong annotation",
	"E1053: Annotation's parameter unstated",
	"E1054: Annotation's value unstated",
	"E1055: Annotation's parameter is unknown",
	"E1056: Annotation's value is unknown",
	"E1101: Floating point number has improper postfix; f32/f64 are possible postfixes",
	"E1102: Number has improper postfix; i/u/f32/64 or i/u8/16 are possible postfixes",
	"E1103: String has improper postfix; str8/16/32 are possible postfixes",
	"E1104: Character has improper postfix; c8/16/32 are possible postfixes",
	"E1105: Number has unknown numeric system; 0x (hex), 0o (oct), 0b (bin) are possible systems",
	"E1106: Number cannot have delimiter here",
	"E1107: Inconsistent hexadecimal number; using both upper and lower cases is forbidden",
	"E1108: String contains character(s) that are not allowed by its format "\
		"(e.g. ascii string with characters which code is bigger than 255",
	"E1111: Character literal has no closing apostrophe (')",

	"E2001: No expression found where it is expected to be",
	"E2002: Unexpected token",

	"E3001: ",

	"E4001: String conversion error; cannot convert string from utf32 when its size is not multiple of 4",
	"E4051: Loading module symbols twice; symbols of the module with such name were already loaded",
	"E4052: No module found with such alias; trying to get a symbol of module with a wrong alias",
	"E4053: No module found with such path; trying to set nullptr as the current module"
};

std::string errorIDToString(ErrorID id) {
	return ErrorIDStrings[u32(id)];
}

namespace ErrorManager {
	void printError(const std::string& error, int line, const std::string& msg);
}

void ErrorManager::init(OutputMode mode, const std::string& log_file) {
	_mode = mode;
	if (mode & LOGS) {
		_file = new std::ofstream(log_file);
	} else {
		_file = nullptr;
	}
}

void ErrorManager::clear() {
	_mode = NO_MODE;
	if (_file != nullptr) {
		delete _file;
		_file = nullptr;
	}
}

void ErrorManager::lexerError(ErrorID id, int line, const std::string& data) {
	ASSERT(id >= ErrorID::E1001_NO_SUCH_MODULE_FOUND && id < ErrorID::E2001_EXPRESSION_NOT_FOUND, "Not a lexer error");
	printError("Lexer error " + errorIDToString(id), line, data);
}

void ErrorManager::parserError(ErrorID id, int line, const std::string& data) {
	ASSERT(id >= ErrorID::E2001_EXPRESSION_NOT_FOUND && id < ErrorID::E3001, "Not a parser error");
	printError("Parser error " + errorIDToString(id), line, data);
}

void ErrorManager::typeError(ErrorID id, int line, const std::string& data) {
	ASSERT(id >= ErrorID::E3001 && id < ErrorID::E4001_WRONGLY_READ_STRING_BAD_SIZE, "Not a type error");
	printError("Type error " + errorIDToString(id), line, data);
}

void ErrorManager::internalError(ErrorID id, int line, const std::string& data) {
	ASSERT(id >= ErrorID::E4001_WRONGLY_READ_STRING_BAD_SIZE, "Not an internal error");
	printError("Internal error " + errorIDToString(id), line, data);
}

void ErrorManager::warning(ErrorID id, const std::string& data) {
	ASSERT(id < ErrorID::E1001_NO_SUCH_MODULE_FOUND, "Not a warning");
	std::string text = "\a";
	text.append("Warning " + errorIDToString(id) + " in " + g_currFileName + ": " + data);

	if (_mode & CONSOLE) {
		std::cout << text << std::endl;
	} if (_mode & LOGS) {
		ASSERT(_file == nullptr, "");
		(*_file) << text << std::endl;
	}
}

void ErrorManager::printError(const std::string& error, int line, const std::string& msg) {
	std::string text = "\a";
	text.append(error + " in " + g_currFileName + ", line " + std::to_string(line + 1) + ": ");
	text.append(msg);

	if (_mode & CONSOLE) {
		std::cout << text << std::endl;
	} if (_mode & LOGS) {
		ASSERT(_file == nullptr, "");
		(*_file) << text << std::endl;
	}
}
