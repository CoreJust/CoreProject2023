#pragma once
#include <fstream>
#include <string>
#include <iostream>
#include <Utils/Defs.h>

// The file currently processed. Must be changed outside this module.
extern std::string g_currFileName;
extern std::string g_currFilePath; // format: *:/**/**/**.core

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

	void lexerError(int line, const std::string& data);
	void parserError(int line, const std::string& data);
	void typeError(int line, const std::string& data);
	void internalError(int line, const std::string& data);

	void warning(const std::string& data);
};