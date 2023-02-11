#include <iostream>
#include "Compiler.h"

// TODO: add expressions, add aliases, name mangling, safe/unsafe code
// Long term TODO: project settings file, implement optionals
// Current tasks: refactor the project, add function overloading, add user-defined types, add va_args (in user-defined funcs as well), implement crt
// To test: strings, format strings, str's convertions

int main() {
	Project project;
	Compiler compiler(project);
	compiler.buildProject();
	compiler.linkProject();
	compiler.runProject();

	system("pause");
	quick_exit(0); // so as not to fail due to llvm destructors
	return 0;
}