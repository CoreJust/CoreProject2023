#include <iostream>
#include "Compiler.h"

// TODO: add variables, add type system
// Long term TODO: project settings file
// To test: strings, format strings

int main() {
	Project project;
	Compiler compiler(project);
	compiler.buildProject();
	compiler.linkProject();
	compiler.runProject();

	quick_exit(0);
	return 0;
}