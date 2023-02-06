#include <iostream>
#include "Compiler.h"

// TODO: add variables, add type system (types symbol loading, parsing, in AST, etc), add expressions, add aliases, name mangling
// Long term TODO: project settings file, user-defined types
// To test: strings, format strings, str's convertions

int main() {
	Project project;
	Compiler compiler(project);
	compiler.buildProject();
	compiler.linkProject();
	compiler.runProject();

	quick_exit(0); // so as not to fail due to llvm destructors
	return 0;
}