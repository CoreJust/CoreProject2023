#include <iostream>
#include <Project/Compiler.h>

// TODO: refactor operation expressions, arguments' default values
// Long term TODO: implement optionals, add ct preprocesing
// Current tasks: templates, internal types, llvm intrinsics, destructors, ranges (for ranges)
// To test: strings, format strings, str's convertions

int main() {
	Project project("C:/Users/egor2/source/repos/CoreProject2023/examples/testProject.coreproject");
	Compiler compiler(project);
	compiler.buildProject();
	compiler.linkProject();

	if (project.getSettings().compilationMode == CompilationMode::Program) {
		char ch;
		std::cout << "\nProject built; enter y to run it: ";
		std::cin >> ch;

		if (ch == 'y' || ch == 'Y') {
			compiler.runProject();
		}
	}

	quick_exit(0); // so as not to fail due to llvm destructors, to fix
	return 0;
}