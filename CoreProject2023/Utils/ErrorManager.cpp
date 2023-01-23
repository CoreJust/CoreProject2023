#include "ErrorManager.h"

std::string g_currFileName = "";
std::string g_currFilePath = "";

std::ofstream* ErrorManager::_file;
ErrorManager::OutputMode ErrorManager::_mode;

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

void ErrorManager::lexerError(int line, const std::string& data) {
	printError("Lexer error", line, data);
}

void ErrorManager::parserError(int line, const std::string& data) {
	printError("Parser error", line, data);
}

void ErrorManager::typeError(int line, const std::string& data) {
	printError("Type error", line, data);
}

void ErrorManager::internalError(int line, const std::string& data) {
	printError("Internal error", line, data);
}

void ErrorManager::warning(const std::string& data) {
	if (_mode & CONSOLE) {
		std::cout << "Warning in " << g_currFileName << ": " << data << std::endl;
	} if (_mode & LOGS) {
		ASSERT(_file == nullptr, "");
		(*_file) << "Warning in " << g_currFileName << ": " << data << std::endl;
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
