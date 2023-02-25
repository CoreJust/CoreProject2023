#include "ErrorManager.h"
#include "String.h"

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
	"E1112: String literal has no closing quote (\"/\"\"\")",


	"E2001: No expression found where it is expected to be",
	"E2002: Unexpected token",
	"E2003: Unknown identifier; there is no such variable/function",
	"E2004: There are multiple functions with such name; specify its argument types",
	"E2005: No function exists that can be called with such arguments",
	"E2006: No member of the type with such name exists or the member is inaccessible",
	"E2007: No access to the member",
	"E2008: Incorrect array element access expression",
	"E2009: Too many array elements",

	"E2101: Variable declared via keyword var must be initialized",
	"E2102: Expression cannot be called; no operator () defined",
	"E2103: Expression is not a reference; cannot be modified",
	"E2104: Function does not have a body",
	"E2105: Varying arguments (...) can only be the last argument of a function",
	"E2106: Keyword 'this' can only be used inside methods/constructors/destructors",
	"E2107: Type constructor cannot be static",
	"E2108: Impossible number of arguments of operator-function",
	"E2109: Conditional operator must return bool",

	"E2201: Unsafe code met in a safe-only code: remove the unsafe code or mark it as safe",


	"E3001: Type not specified; type expression expected",
	"E3002: Unexpected token while parsing a type",
	"E3003: Not a type; type expression expected",

	"E3051: Reference to a reference; impossible syntax",
	"E3052: Non-positive size array; array must have size bigger than 0",
	"E3053: Cannot dereference; not a pointer",
	"E3054: Cannot negate an unsigned int; value cannot be lesser then 0",
	"E3055: Cannot get a reference from the value",
	"E3056: Must be a reference",
	"E3057: Value is const",

	"E3101: Type cannot be implicitly converted",
	"E3102: Type cannot be explicitly converted",
	"E3103: Types cannot be converted to single type",
	"E3103: Constructor not found",


	"E4001: String conversion error; cannot convert string from utf32 when its size is not multiple of 4",
	"E4051: Loading module symbols twice; symbols of the module with such name were already loaded",
	"E4052: No module found with such alias; trying to get a symbol of module with a wrong alias",
	"E4053: No module found with such path; trying to set nullptr as the current module",


	"E5001: Unknown project setting",
	"E5002: Project setting has wrong type",
	"E5003: Project setting has impossible value",
	"E5004: Project setting: failed to load",
	"E5005: A necessary setting is missed",
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
	ASSERT(id >= ErrorID::E1001_NO_SUCH_MODULE_FOUND
		&& id < ErrorID::E2001_EXPRESSION_NOT_FOUND,
		"Not a lexer error");

	printError("Lexer error " + errorIDToString(id), line, data);
}

void ErrorManager::parserError(ErrorID id, int line, const std::string& data) {
	ASSERT(id >= ErrorID::E2001_EXPRESSION_NOT_FOUND
		&& id < ErrorID::E3001_TYPE_NOT_SPECIFIED,
		"Not a parser error");

	printError("Parser error " + errorIDToString(id), line, data);
}

void ErrorManager::typeError(ErrorID id, int line, const std::string& data) {
	ASSERT(id >= ErrorID::E3001_TYPE_NOT_SPECIFIED
		&& id < ErrorID::E4001_WRONGLY_READ_STRING_BAD_SIZE,
		"Not a type error");

	printError("Type error " + errorIDToString(id), line, data);
}

void ErrorManager::internalError(ErrorID id, int line, const std::string& data) {
	ASSERT(id >= ErrorID::E4001_WRONGLY_READ_STRING_BAD_SIZE
		&& id < ErrorID::E5001_UNKNOWN_SETTING,
		"Not an internal error");

	printError("Internal error " + errorIDToString(id), line, data);
}

void ErrorManager::projectSettingsError(ErrorID id, int line, const std::string& data) {
	ASSERT(id >= ErrorID::E5001_UNKNOWN_SETTING,
		"Not a project settings error");

	printError("Project settings error " + errorIDToString(id), line, data);
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

	printFileLine(line);

	system("pause");
	quick_exit(0);
}

void ErrorManager::printFileLine(int line) {
	if (line < 0) {
		return;
	}

	std::ifstream file(g_currFilePath);
	std::string tmp;
	size_t i = 0;
	while (std::getline(file, tmp)) {
		if (i == line) {
			if (_mode & CONSOLE) {
				std::cout << tmp << std::endl;
			} if (_mode & LOGS) {
				ASSERT(_file == nullptr, "");
				(*_file) << tmp << std::endl;
			}

			break;
		}

		if (size_t off = std::count(tmp.begin(), tmp.end(), '\r')) {
			if (i + off >= line) {
				std::vector<std::string> splitted = split(tmp, '\r');
				if (_mode & CONSOLE) {
					std::cout << splitted[line - i] << std::endl;
				} if (_mode & LOGS) {
					ASSERT(_file == nullptr, "");
					(*_file) << splitted[line - i] << std::endl;
				}

				break;
			}

			i += off;
		}

		i++;
	}

	file.close();
}
