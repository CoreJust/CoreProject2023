#include <iostream>
#include "Compiler.h"

// TODO: add variables, fix destruction of g_context, add type system
// Long term TODO: project settings file
// To test: strings, format strings

int main() {
	Project project;
	Compiler compiler(project);
	compiler.buildProject();
	compiler.linkProject();
	compiler.runProject();

	return 0;
}