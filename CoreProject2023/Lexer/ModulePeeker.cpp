#include "ModulePeeker.h"
#include "ImportsHandler.h"

std::set<std::string> g_peekedModules;


ModulePeeker::ModulePeeker(std::string modulePath)
	: m_modulePath(std::move(modulePath)) {

}

ModuleRef ModulePeeker::load() {
	if (g_peekedModules.contains(m_modulePath)) {
		return g_moduleList.getModule(m_modulePath);
	}

	g_peekedModules.insert(m_modulePath);

	// Peeking into the current module
	m_file.open(m_modulePath);
	std::getline(m_file, m_text);

	processModuleQualities();
	processImports();
	m_file.close();

	// Recursive peeking into imported modules
	for (auto& import : m_imports) {
		if (!g_peekedModules.contains(import)) {
			ModulePeeker(import).load();
		}
	}

	Module thisModule(
		Module::getModuleNameFromPath(m_modulePath),
		m_modulePath,
		m_qualities,
		std::move(m_imports)
	);

	thisModule.loadImportsList();
	return g_moduleList.addModule(std::move(thisModule));
}

void ModulePeeker::processModuleQualities() {
	// read annotations
	while (m_pos < m_text.size()) {
		skipWhitespaces(false);
		if (m_pos < m_text.size() && m_text[m_pos] == '#') {
			skipComment();
			continue;
		}

		if (m_pos >= m_text.size() - 3 || m_text[m_pos] != '@'
			|| m_text[m_pos + 1] != 's' || m_text[m_pos + 2] != 'e' || m_text[m_pos + 3] != 't') break;

		// skip @set
		next();
		next();
		next();
		next();

		skipWhitespaces(true);
		if (m_pos >= m_text.size() || !(isalnum(m_text[m_pos]) || m_text[m_pos] == '_' || m_text[m_pos] == '&'))
			ErrorManager::lexerError(
				ErrorID::E1053_ANNOTATION_PARAMETER_UNSTATED,
				m_line,
				"the parameter of @set is unstated"
			);

		std::string parameter;
		loadIdentifier(parameter);

		skipWhitespaces(true);
		if (m_pos >= m_text.size() || !(isalnum(m_text[m_pos]) || m_text[m_pos] == '_' || m_text[m_pos] == '&'))
			ErrorManager::lexerError(
				ErrorID::E1054_ANNOTATION_VALUE_UNSTATED,
				m_line,
				"the value of @set is unstated"
			);

		m_buffer.clear();
		loadIdentifier(m_buffer);

		// parse the annotation
		if (parameter == "visibility") {
			if (m_buffer == "public") {
				m_qualities.setVisibility(Visibility::PUBLIC);
			} else if (m_buffer == "private") {
				m_qualities.setVisibility(Visibility::PRIVATE);
			} else if (m_buffer == "direct_import") {
				m_qualities.setVisibility(Visibility::DIRECT_IMPORT);
			} else {
				ErrorManager::lexerError(
					ErrorID::E1056_UNKNOWN_ANNOTATION_VALUE,
					m_line,
					"@set visibility " + m_buffer
				);
			}
		} else if (parameter == "safety") {
			if (m_buffer == "safe") {
				m_qualities.setSafety(Safety::SAFE);
			} else if (m_buffer == "unsafe") {
				m_qualities.setSafety(Safety::UNSAFE);
			} else if (m_buffer == "safe_only") {
				m_qualities.setSafety(Safety::SAFE_ONLY);
			} else {
				ErrorManager::lexerError(
					ErrorID::E1056_UNKNOWN_ANNOTATION_VALUE,
					m_line,
					"@set safety " + m_buffer
				);
			}
		}
		else if (parameter == "mangling") {
			if (m_buffer == "mangle") {
				m_qualities.setMangling(true);
			} else if (m_buffer == "nomangle") {
				m_qualities.setMangling(false);
			} else {
				ErrorManager::lexerError(
					ErrorID::E1056_UNKNOWN_ANNOTATION_VALUE,
					m_line,
					"@set mangling " + m_buffer
				);
			}
		} else {
			ErrorManager::lexerError(
				ErrorID::E1055_UNKNOWN_ANNOTATION_PARAMETER,
				m_line,
				"@set " + m_buffer
			);
		}
	}
}

void ModulePeeker::processImports() {
	ImportsHandler imports;
	while (m_pos < m_text.size()) {
		skipWhitespaces(false);
		if (m_pos < m_text.size() && m_text[m_pos] == '#') {
			skipComment();
			continue;
		}

		if (m_pos < m_text.size() && m_text[m_pos] == 'i') {
			loadIdentifier(m_buffer);
		} else {
			break;
		}

		if (m_buffer == "import") {
			// read the module to import
			m_buffer.clear();
			skipWhitespaces(true);

			while (m_pos < m_text.size() && m_text[m_pos] != ';') {
				if (m_text[m_pos] == '\n' || m_text[m_pos] == '\r') {
					ErrorManager::lexerError(
						ErrorID::E1002_NO_ENDING_SEMICOLON,
						m_line,
						"import " + m_buffer
					);
				}

				m_buffer += m_text[m_pos]; // read the module to import
				next();
			}

			next(); // skip ;
			skipWhitespaces(false);

			imports.addImport(m_buffer);
		} else {
			break;
		}
	}

	m_imports = imports.getImportedFiles();
}

void ModulePeeker::skipComment() {
	if (m_pos < m_text.size() - 6 && m_text[m_pos + 1] == '#' && m_text[m_pos + 2] == '#') { // multiline comment
		next();
		next();
		next();
		while (true) {
			if (m_pos == m_text.size()) {
				ErrorManager::lexerError(
					ErrorID::E1003_MULTILINE_COMMENT_IS_NOT_CLOSED,
					m_line,
					""
				);
			}

			if (m_text[m_pos] == '"') {
				skipString();
			}

			if (m_pos < m_text.size() - 2) {
				if (m_text[m_pos] == '#' && m_text[m_pos + 1] == '#' && m_text[m_pos + 2] == '#') {
					next();
					next();
					next();
					return;
				}
			}

			next();
		}
	}

	// single-line comment
	char c = next();
	while (c != '\n' && c != '\0' && c != '\r') {
		c = next();
	}
}

void ModulePeeker::skipString() {
	if (next() == '"') {
		if (next() == '"') { // multiline string
			while (true) {
				if (m_pos >= m_text.size()) { // eof
					ErrorManager::lexerError(
						ErrorID::E1112_NO_CLOSING_QUOTE,
						m_line,
						"no closing quote till the end of file"
					);
				}

				if (next() == '"' && next() == '"' && next() == '"') {
					return;
				}
			}
		}
		else {
			m_pos--;
			return;
		}
	}
	else { // single-line string
		while (next() != '"');
	}
}

void ModulePeeker::loadIdentifier(std::string& to) {
	m_buffer.clear();

	char c = m_text[m_pos];
	do {
		to += c;
		c = next();
	} while (isalnum(c) || c == '_' || c == '$');
}

void ModulePeeker::skipWhitespaces(bool spacesOnly) {
	static std::string allSpaces = " \t\n\r\f\v";
	if (spacesOnly) {
		while (m_pos < m_text.size() && (m_text[m_pos] == ' ' || m_text[m_pos] == '\t')) {
			next(); // skip whitespaces
		}
	}
	else {
		while (m_pos < m_text.size() && allSpaces.find(m_text[m_pos]) != std::string::npos) {
			next(); // skip whitespaces
		}
	}
}

char ModulePeeker::next() {
	m_pos++;
	if (m_line < m_nextLine) {
		m_line = m_nextLine;
	}

	if (m_pos >= m_text.size() - 1) {
		std::string tmp;
		if (std::getline(m_file, tmp)) {
			m_text += '\n';
			m_text += tmp;
		}
	}

	if (m_pos >= m_text.size()) {
		return '\0';
	}

	char r = m_text[m_pos];
	if (r == '\n' || r == '\r') {
		m_nextLine++;
	}

	return r;
}
