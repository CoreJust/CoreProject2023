#include "Lexer.h"
#include <map>
#include <functional>
#include <Utils/String.h>
#include "ImportsHandler.h"

std::vector<std::string> KEY_WORDS{
	"import", "use",
	"def", "native", "return",
	"class", "interface", "struct", "enum", "union", "tuple",
	"public", "private", "protected", "abstract", "virtual", "this",
	"const", "var", "extern", "static",
	"unsafe", "ct",
	"if", "else", "elif",
	"switch", "match", "case", "default",
	"for", "while", "do",
	"break", "continue",
	"true", "false", "null",
	"new", "delete", "in", "is", "as", "typeof", "ref", "move",
	"bool", "c8", "c16", "c32", "str8", "str16", "str32", "i8", "i16", "i32", "i64",
	"u8", "u16", "u32", "u64", "f32", "f64", "func"
};

std::map<std::string, TokenType> OPERATORS = {
	{ "=", TokenType::EQ },
	{ "+=", TokenType::PLUS_EQ },
	{ "-=", TokenType::MINUS_EQ },
	{ "*=", TokenType::STAR_EQ },
	{ "/=", TokenType::SLASH_EQ },
	{ "//=", TokenType::DSLASH_EQ },
	{ "%=", TokenType::PERCENT_EQ },
	{ "**=", TokenType::POWER_EQ },
	{ "&=", TokenType::AND_EQ },
	{ "|=", TokenType::OR_EQ },
	{ "^=", TokenType::XOR_EQ },
	{ "<<=", TokenType::LSHIFT_EQ },
	{ ">>=", TokenType::RSHIFT_EQ },
	{ "++", TokenType::INCREMENT },
	{ "--", TokenType::DECREMENT },
	{ "+", TokenType::PLUS },
	{ "-", TokenType::MINUS },
	{ "*", TokenType::STAR },
	{ "/", TokenType::SLASH },
	{ "//", TokenType::DSLASH },
	{ "%", TokenType::PERCENT },
	{ "**", TokenType::POWER },
	{ "&", TokenType::AND },
	{ "|", TokenType::OR },
	{ "^", TokenType::XOR },
	{ "<<", TokenType::LSHIFT },
	{ ">>", TokenType::RSHIFT },
	{ "~", TokenType::TILDE },
	{ "!", TokenType::EXCL },
	{ "!=", TokenType::EXCLEQ },
	{ "<", TokenType::LESS },
	{ ">", TokenType::GREATER },
	{ "==", TokenType::EQEQ },
	{ "<=", TokenType::LESSEQ },
	{ ">=", TokenType::GREATEREQ },
	{ "&&", TokenType::ANDAND },
	{ "||", TokenType::OROR },
	{ "(", TokenType::LPAR },
	{ ")", TokenType::RPAR },
	{ "{", TokenType::LBRACE },
	{ "}", TokenType::RBRACE },
	{ "[", TokenType::LBRACKET },
	{ "]", TokenType::RBRACKET },
	{ ",", TokenType::COMMA },
	{ ".", TokenType::DOT },
	{ "..", TokenType::RANGEDOT },
	{ "...", TokenType::ETCETERA },
	{ "?", TokenType::QUESTION },
	{ ":", TokenType::COLON },
	{ ";", TokenType::SEMICOLON },
	{ "@", TokenType::AT }
};

const std::string OP_CHARS = "=+-*/%&|^<>~!(){}[],.?:;@";


Lexer::Lexer(const std::string& text) 
	: m_text(text) {

}

std::vector<Token> Lexer::tokenize() {
	while (m_pos < m_text.size()) {
		nextToken();
	}

	return std::move(m_toks);
}

ModuleQualities Lexer::handleModuleQualities() {
	ModuleQualities result;

	// read annotations
	while (m_pos < m_text.size()) {
		skipWhitespaces(false);
		if (m_pos < m_text.size() && m_text[m_pos] == '#') {
			tokenizeComment();
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
				result.setVisibility(Visibility::PUBLIC);
			} else if (m_buffer == "private") {
				result.setVisibility(Visibility::PRIVATE);
			} else if (m_buffer == "direct_import") {
				result.setVisibility(Visibility::DIRECT_IMPORT);
			} else {
				ErrorManager::lexerError(
					ErrorID::E1056_UNKNOWN_ANNOTATION_VALUE,
					m_line,
					"@set visibility " + m_buffer
				);
			}
		} else if (parameter == "safety") {
			if (m_buffer == "safe") {
				result.setSafety(Safety::SAFE);
			} else if (m_buffer == "unsafe") {
				result.setSafety(Safety::UNSAFE);
			} else if (m_buffer == "safe_only") {
				result.setSafety(Safety::SAFE_ONLY);
			} else {
				ErrorManager::lexerError(
					ErrorID::E1056_UNKNOWN_ANNOTATION_VALUE, 
					m_line, 
					"@set safety " + m_buffer
				);
			}
		} else if (parameter == "mangling") {
			if (m_buffer == "mangle") {
				result.setMangling(true);
			} else if (m_buffer == "nomangle") {
				result.setMangling(false);
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

	return result;
}

std::vector<std::string> Lexer::handleImports() {
	ImportsHandler imports;
	while (m_pos < m_text.size()) {
		skipWhitespaces(false);
		if (m_pos < m_text.size() && m_text[m_pos] == '#') {
			tokenizeComment();
			continue;
		}

		if (m_pos < m_text.size() && m_text[m_pos] == 'i')
			tokenizeWord();
		else
			break;

		if (m_toks.back().type == TokenType::IMPORT) {
			m_toks.pop_back();

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

	return imports.getImportedFiles();
}

void Lexer::nextToken() {
	if (isdigit(m_text[m_pos])) {
		tokenizeNumber();
	} else if (isalpha(m_text[m_pos]) || m_text[m_pos] == '_') {
		tokenizeWord();
	} else if (m_text[m_pos] == '\'') {
		tokenizeCharacter();
	} else if (m_text[m_pos] == '\"' ||
		(m_pos < m_text.size() - 1 && m_text[m_pos] == 'f' && m_text[m_pos + 1] == '\"')) {
		tokenizeText();
	} else if (m_text[m_pos] == '#') {
		tokenizeComment();
	} else if (isOperator(m_text[m_pos])) {
		tokenizeOperator();
	} else {
		next();
	}
}

void Lexer::tokenizeNumber() {
	char c = m_text[m_pos];

	// Number system (hex, binary, etc)
	int number_system = 10;
	if (c == '0') {
		c = next();
		switch (c) {
			case 'x': number_system = 16; break;
			case 'o': number_system = 8; break;
			case 'b': number_system = 2; break;
		default: break;
		}

		if (number_system != 10) {
			c = next();
		} else {
			c = m_text[--m_pos];
		}
	}

	loadNumber(number_system, true, true);
	if (m_text[m_pos] == 'i' || m_text[m_pos] == 'u' || m_text[m_pos] == 'f') {
		std::string type;
		loadIdentifier(type);

		if (m_buffer.find('.') != std::string::npos && type[0] != 'f') {
			ErrorManager::lexerError(
				ErrorID::E1101_FLOAT_LITERAL_IMPROPER_POSTFIX,
				m_line, 
				"postfix: " + type
			);
		}

		if (type == "f32") {
			return m_toks.push_back(Token(TokenType::NUMBERF32, m_buffer, m_line));
		} else if (type == "f64") {
			return m_toks.push_back(Token(TokenType::NUMBERF64, m_buffer, m_line));
		} else if (type == "i8") {
			return m_toks.push_back(Token(TokenType::NUMBERI8, m_buffer, m_line));
		} else if (type == "i16") {
			return m_toks.push_back(Token(TokenType::NUMBERI16, m_buffer, m_line));
		} else if (type == "i32") {
			return m_toks.push_back(Token(TokenType::NUMBERI32, m_buffer, m_line));
		} else if (type == "i64") {
			return m_toks.push_back(Token(TokenType::NUMBERI64, m_buffer, m_line));
		} else if (type == "u8") {
			return m_toks.push_back(Token(TokenType::NUMBERU8, m_buffer, m_line));
		} else if (type == "u16") {
			return m_toks.push_back(Token(TokenType::NUMBERU16, m_buffer, m_line));
		} else if (type == "u32") {
			return m_toks.push_back(Token(TokenType::NUMBERU32, m_buffer, m_line));
		} else if (type == "u64") {
			return m_toks.push_back(Token(TokenType::NUMBERU64, m_buffer, m_line));
		} else {
			ErrorManager::lexerError(
				ErrorID::E1102_NUMBER_LITERAL_IMPROPER_POSTFIX, 
				m_line, 
				m_buffer + " " + type
			);
		}
	}

	if (m_buffer.find('.') != std::string::npos) {
		return m_toks.push_back(Token(TokenType::NUMBERF32, m_buffer, m_line));
	}
	
	if (m_buffer[0] == '-') {
		i64 num = std::stoll(m_buffer);
		if (num <= INT32_MAX) {
			return m_toks.push_back(Token(TokenType::NUMBERI32, m_buffer, m_line));
		} else {
			return m_toks.push_back(Token(TokenType::NUMBERI64, m_buffer, m_line));
		}
	} else {
		u64 num = std::stoull(m_buffer);
		if (num <= INT32_MAX) {
			return m_toks.push_back(Token(TokenType::NUMBERI32, m_buffer, m_line));
		} else if (num <= INT64_MAX) {
			return m_toks.push_back(Token(TokenType::NUMBERI64, m_buffer, m_line));
		} else {
			return m_toks.push_back(Token(TokenType::NUMBERU64, m_buffer, m_line));
		}
	}
}

void Lexer::tokenizeOperator() {
	m_buffer.clear();

	char c = m_text[m_pos];
	while (c != '\0') {
		m_buffer += c;
		if (m_buffer.size() > 1) {
			if (OPERATORS.find(m_buffer) == OPERATORS.end()) {
				m_buffer.erase(m_buffer.size() - 1, 1);
				m_toks.push_back(Token(OPERATORS[m_buffer], m_buffer, m_line));
				return;
			}
		}

		c = next();
	}
}

void Lexer::tokenizeText() {
	m_buffer.clear();

	// f"...{code}...", tokens are: FORMAT_TEXT, code tokens, FORMAT_TEXT, ..., FORMAT_STRING_END
	bool isFString = false;

	// for formated strings: the tokens with strings between inserted code parts
	std::vector<size_t> fStringParts;

	if (m_text[m_pos] == 'f') {
		isFString = true;
		next();
	}

	bool isMultiline = false;
	if (m_text[m_pos + 1] == '"' && m_text[m_pos + 2] == '"') {
		isMultiline = true;
		next();
		next();
	}

	while (true) {
		if (m_pos >= m_text.size() - 1) {
			ErrorManager::lexerError(
				ErrorID::E1112_NO_CLOSING_QUOTE, 
				m_line, 
				"no closing quote till the end of file"
			);

			break;
		}

		if (!isMultiline && (m_text[m_pos + 1] == '\n' || m_text[m_pos + 1] == '\r')) {
			ErrorManager::lexerError(
				ErrorID::E1112_NO_CLOSING_QUOTE, 
				m_line, 
				"no closing quote before the end of line"
			);
		} else if (m_text[m_pos + 1] == '"') {
			if (!isMultiline) {
				break;
			} if (m_text[m_pos + 2] == '"' && m_text[m_pos + 3] == '"') {
				next(); // skip first "
				next(); // skip second ", the last is skipped after the cycle
				break;
			}
		}

		if (isFString && m_text[m_pos + 1] == '{') {
			next();
			fStringParts.push_back(m_toks.size());
			m_toks.push_back(Token(TokenType::FORMAT_TEXT8, m_buffer, m_line));

			// Note: Copied from tokenize() function
			while (m_pos < m_text.size() - 1 && m_text[m_pos] != '}') {
				nextToken();
			}

			m_buffer.clear();
			continue;
		}

		addUtf32(m_buffer, getSingleChar());
	}

	next(); // skip the last character
	next(); // skip closing "

	std::function<u32(std::string&)> convertFunc = utf32ToASCII;
	TokenType tok_type = isFString ?
		TokenType::FORMAT_TEXT8
		: TokenType::TEXT8;

	if (m_text[m_pos] == 's') {
		std::string type;
		loadIdentifier(type);

		if (type == "str8") {
			convertFunc = utf32ToASCII;
			tok_type = isFString ?
				TokenType::FORMAT_TEXT8
				: TokenType::TEXT8;
		} else if (type == "str16") {
			convertFunc = utf32ToUtf16;
			tok_type = isFString ?
				TokenType::FORMAT_TEXT16
				: TokenType::TEXT16;
		} else if (type == "str32") {
			convertFunc = [](std::string& str) -> u32 { return 0; };
			tok_type = isFString ?
				TokenType::FORMAT_TEXT32
				: TokenType::TEXT32;
		} else {
			ErrorManager::lexerError(
				ErrorID::E1103_STRING_IMPROPER_POSTFIX, 
				m_line, 
				"postfix: " + type
			);
		}
	}

	if (isFString) {
		for (size_t pos : fStringParts) {
			if (auto errCode = convertFunc(m_toks[pos].data)) {
				printStringTranslationError(errCode);
			}

			m_toks[pos].type = tok_type;
		}

		if (m_buffer.size()) {
			if (auto errCode = convertFunc(m_buffer)) {
				printStringTranslationError(errCode);
			}

			m_toks.push_back(Token(tok_type, m_buffer, m_line));
		}

		m_toks.push_back(Token(TokenType::FORMAT_STRING_END, m_line));
	} else {
		if (auto errCode = convertFunc(m_buffer)) {
			printStringTranslationError(errCode);
		}

		m_toks.push_back(Token(tok_type, m_buffer, m_line));
	}
}

void Lexer::tokenizeCharacter() {
	m_buffer.clear();

	addUtf32(m_buffer, getSingleChar());
	if (next() != '\'') {
		ErrorManager::lexerError(
			ErrorID::E1111_NO_CLOSING_APOSTROPHE, 
			m_line, 
			""
		);
	}

	next(); // skip closing '

	if (m_text[m_pos] == 'c') {
		std::string type;
		loadIdentifier(type);
		if (type == "c8") {
			if (auto errCode = utf32ToASCII(m_buffer)) {
				printStringTranslationError(errCode);
			}

			return m_toks.push_back(Token(TokenType::LETTER8, m_buffer, m_line));
		} else if (type == "c16") {
			if (auto errCode = utf32ToUtf16(m_buffer)) {
				printStringTranslationError(errCode);
			}

			return m_toks.push_back(Token(TokenType::LETTER16, m_buffer, m_line));
		} else if (type == "c32") {
			return m_toks.push_back(Token(TokenType::LETTER32, m_buffer, m_line));
		} else {
			ErrorManager::lexerError(
				ErrorID::E1104_CHARACTER_IMPROPER_POSTFIX, 
				m_line, 
				"unknown character postfix: " + type
			);
		}
	}

	if (auto errCode = utf32ToASCII(m_buffer)) {
		printStringTranslationError(errCode);
	}

	return m_toks.push_back(Token(TokenType::LETTER8, m_buffer, m_line));
}

void Lexer::tokenizeWord() {
	m_buffer.clear();
	loadIdentifier(m_buffer);

	int isKey = isKeyWord(m_buffer);
	if (isKey != -1) {
		TokenType keyWord = TokenType(+TokenType::IMPORT + isKey);
		if (keyWord == TokenType::ELIF) {
			m_toks.push_back(Token(TokenType::ELSE, "else", m_line));
			m_toks.push_back(Token(TokenType::IF, "if", m_line));
		} else {
			m_toks.push_back(Token(keyWord, m_buffer, m_line));
		}
	} else {
		m_toks.push_back(Token(TokenType::WORD, m_buffer, m_line));
	}
}

void Lexer::tokenizeLine() {
	m_buffer.clear();

	char c = next(); // skip whitespace
	while (c != '\n' && c != '\r' && c != '\0' && c != ';') {
		m_buffer += c;
		c = next();
	}

	m_toks.back().data = m_buffer;
}

void Lexer::tokenizeComment() {
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
	while (c != '\n' && c != '\0' && c != '\r')
		c = next();
}

void Lexer::loadIdentifier(std::string& to) {
	char c = m_text[m_pos];
	do {
		to += c;
		c = next();
	} while (isalnum(c) || c == '_' || c == '$');
}

void Lexer::loadNumber(int base, bool allowFloating, bool allowDelimiter) {
	int _case = 0; // 1 if lower case, 2 if upper case
	int firstWrongCase = -1;
	auto& pos = m_pos;
	auto binIsDigit = [](char c) -> int { return c == '0' || c == '1'; };
	auto octIsDigit = [](char c) -> int { return c >= '0' && c <= '7'; };
	auto decIsDigit = [](char c) -> int { return isdigit(c); };
	auto hexIsDigit = [&_case, &firstWrongCase, &pos](char c) -> int {
		if (!isdigit(c) && isxdigit(c)) { // number can consist letters only of the same case
			if (!_case) {
				_case = islower(c) ? 1 : 2;
			} else {
				if (firstWrongCase == -1 && (islower(c) ? _case == 2 : _case == 1)) {
					firstWrongCase = pos;
				}
			}
		}

		return isxdigit(c);
	};

	std::function<int(char)> isDigit;
	switch (base) {
		case 2: isDigit = binIsDigit; break;
		case 8: isDigit = octIsDigit; break;
		case 10: isDigit = decIsDigit; break;
		case 16: isDigit = hexIsDigit; break;
		default:
			ErrorManager::lexerError(
				ErrorID::E1105_NUMBER_HAS_UNKNOWN_NUMERIC_SYSTEM, 
				m_line,
				"numeric system : " + std::to_string(base)
			);
	}

	m_buffer.clear();
	char c = m_text[m_pos];
	do {
		if (c != '\'') {
			m_buffer += c;
		} else if (!allowDelimiter) {
			ErrorManager::lexerError(
				ErrorID::E1106_NUMBER_DELIMITER_FORBIDDEN, 
				m_line, 
				""
			);
		}

		c = next();
	} while (isDigit(c) || (c == '\'' && allowDelimiter));

	// handle floating point like "1.2345"
	if (c == '.') {
		m_buffer += c;
		c = next();
		while (isDigit(c) || (c == '\'' && allowDelimiter)) {
			if (c != '\'') {
				m_buffer += c;
			} else if (!allowDelimiter) {
				ErrorManager::lexerError(
					ErrorID::E1106_NUMBER_DELIMITER_FORBIDDEN, 
					m_line, 
					""
				);
			}

			c = next();
		}
	}

	// check for case consistency and search for a type ending (aka f32/f64).
	if (base == 16) {
		if (m_buffer.size() > 3 && m_buffer[m_buffer.size() - 3] == 'f'
			&& ((m_buffer[m_buffer.size() - 2] == '3' && m_buffer[m_buffer.size() - 1] == '2')
			|| (m_buffer[m_buffer.size() - 2] == '6' && m_buffer[m_buffer.size() - 1] == '4'))
		) {
			m_buffer.resize(m_buffer.size() - 3); // erase the type ending from value section
			m_pos -= 3;
		}

		if (firstWrongCase != -1 && firstWrongCase < m_pos) {
			ErrorManager::lexerError(
				ErrorID::E1107_INCONSISTENT_HEX_NUMBER, 
				m_line, 
				m_buffer
			);
		}
	}

	toDecimal(m_buffer, base);
}

u32 Lexer::getSingleChar() {
	char c = next();

	if (c == '\\') {
		char b = next();
		switch (b) {
			case '\'':
				return '\'';
			case '\"':
				return '\"';
			case '{':
				return '{'; // for formated strings
			case 'n':
				return '\n';
			case 't':
				return '\t';
			case 'a':
				return '\a';
			case 'b':
				return '\b';
			case 'r':
				return '\r';
			case 'f':
				return '\f';
			case 'v':
				return '\v';
			case '0':
				return '\0';
			case '\\':
				return '\\';
			case 'x':
				loadNumber(16, false, false);
				toDecimal(m_buffer, 16);
				return u32(std::stoi(m_buffer));
			case 'd':
				loadNumber(10, false, false);
				return u32(std::stoi(m_buffer));
			case 'o':
				loadNumber(8, false, false);
				toDecimal(m_buffer, 8);
				return u32(std::stoi(m_buffer));
			default:
				return b;
		}
	}

	return c;
}

void Lexer::skipWhitespaces(bool spacesOnly) {
	static std::string allSpaces = " \t\n\r\f\v";
	if (spacesOnly) {
		while (m_pos < m_text.size() && (m_text[m_pos] == ' ' || m_text[m_pos] == '\t')) {
			next(); // skip whitespaces
		}
	} else {
		while (m_pos < m_text.size() && allSpaces.find(m_text[m_pos]) != std::string::npos) {
			next(); // skip whitespaces
		}
	}
}

void Lexer::skipString() {
	while (next() != '"');
}

void Lexer::printStringTranslationError(u32 errCode) {
	if (errCode == 1) {
		ErrorManager::internalError(
			ErrorID::E4001_WRONGLY_READ_STRING_BAD_SIZE, 
			m_line, 
			"wrong string type: cannot convert"
		);
	} else {
		ErrorManager::lexerError(
			ErrorID::E1108_STRING_CONTAINS_CHARS_GREATER_THAN_FORMAT_ALLOWS, 
			m_line,
			"cannot cast the character to shorter code: " + std::to_string(errCode)
		);
	}
}

int Lexer::isKeyWord(std::string& s) {
	for (int i = 0; i < KEY_WORDS.size(); i++) {
		if (s == KEY_WORDS[i]) {
			return i;
		}
	}

	return -1;
}

bool Lexer::isOperator(char ch) {
	for (auto c : OP_CHARS) {
		if (ch == c) {
			return true;
		}
	}

	return false;
}

char Lexer::next() {
	m_pos++;
	if (m_line < m_nextLine) {
		m_line = m_nextLine;
	}

	if (m_pos < m_text.size()) {
		char r = m_text[m_pos];
		if (r == '\n' || r == '\r') {
			m_nextLine++;
		}

		return r;
	} else {
		return '\0';
	}
}
