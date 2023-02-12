#include <iostream>
#include "Compiler.h"

// TODO: add expressions, add aliases, safe/unsafe code, refactor operation expressions, arguments' default values
// Long term TODO: project settings file, implement optionals, add ct preprocesing
// Current tasks: add user-defined types, add va_args (in user-defined funcs as well), implement crt
// To test: strings, format strings, str's convertions

int main() {
	Project project;
	Compiler compiler(project);
	compiler.buildProject();
	compiler.linkProject();

	char ch;
	std::cout << "\nProject built; enter y to run it: ";
	std::cin >> ch;

	if (ch == 'y' || ch == 'Y') {
		compiler.runProject();
	}

	quick_exit(0); // so as not to fail due to llvm destructors
	return 0;
}