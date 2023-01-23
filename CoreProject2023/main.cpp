#include <iostream>
#include <Utils/File.h>
#include <Lexer/Lexer.h>

int main() {
	ErrorManager::init(ErrorManager::CONSOLE);
	g_currFilePath = "C:/Users/egor2/source/repos/CoreProject2023/examples/";

	std::string program = readFile("../examples/test.core");
	Lexer lexer(program);
	auto imports = lexer.handleImports();
	auto toks = lexer.tokenize();

	return 0;
}