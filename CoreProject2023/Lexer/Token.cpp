#include "Token.h"

static std::string TOKEN_NAMES[] = {
	"NUMBERI8", "NUMBERU8", "NUMBERI16", "NUMBERU16", "NUMBERI32", "NUMBERU32", "NUMBERI64", "NUMBERU64", "NUMBERF32", "NUMBERF64",
	"LETTER8", "LETTER16", "LETTER32", "TEXT8", "TEXT16", "TEXT32", "FORMAT_TEXT8", "FORMAT_TEXT16", "FORMAT_TEXT32", 
	"FORMAT_STRING_END", "WORD",
	"IMPORT", "USE", "DEF", "NATIVE", "RETURN", "CLASS", "INTERFACE", "STRUCT", "ENUM", "UNION", "TUPLE",
	"PUBLIC", "PRIVATE", "PROTECTED", "ABSTRACT", "VIRTUAL", "THIS", "CONST", "VAR", "EXTERN", "STATIC",
	"UNSAFE", "CT", "IF", "ELSE", "ELIF", "SWITCH", "MATCH", "CASE", "DEFAULT", "FOR", "WHILE", "DO", "BREAK", "CONTINUE",
	"TRUE", "FALSE", "NULLPTR", "NEW", "DELETE", "IN", "IS", "AS", "TYPEOF", "MOVE", "TYPE",
	"BOOL", "C8", "C16", "C32", "STR8", "STR16", "STR32", "I8", "I16", "I32", "I64", "U8", "U16", "U32", "U64", "F32", "F64", "FUNC",
	"EQ", "PLUS_EQ", "MINUS_EQ", "STAR_EQ", "SLASH_EQ", "DSLASH_EQ", "PERCENT_EQ", "POWER_EQ", "AND_EQ", "OR_EQ", "XOR_EQ",
	"LSHIFT_EQ", "RSHIFT_EQ", "INCREMENT", "DECREMENT",
	"PLUS", "MINUS", "STAR", "SLASH", "DSLASH", "PERCENT", "POWER", "AND", "OR", "XOR", "LSHIFT", "RSHIFT", "TILDE",
	"EXCL", "EXCLEQ", "LESS", "GREATER", "EQEQ", "LESSEQ", "GREATEREQ", "ANDAND", "OROR", "LPAR", "RPAR", "LBRACE", "RBRACE",
	"LBRACKET", "RBRACKET", "COMMA", "DOT", "RANGEDOT", "ETCETERA", "QUESTION", "COLON", "SEMICOLON", "AT",
	"NO_TOKEN"
};

bool isPossibleNumArgumentsOfOperator(TokenType op, u32 num) {
	switch (op ) {
		case TokenType::IN:
		case TokenType::IS:
		case TokenType::EQ:
		case TokenType::PLUS_EQ:
		case TokenType::MINUS_EQ:
		case TokenType::STAR_EQ:
		case TokenType::SLASH_EQ:
		case TokenType::DSLASH_EQ:
		case TokenType::PERCENT_EQ:
		case TokenType::POWER_EQ:
		case TokenType::AND_EQ:
		case TokenType::OR_EQ:
		case TokenType::XOR_EQ:
		case TokenType::LSHIFT_EQ:
		case TokenType::RSHIFT_EQ:
		case TokenType::DSLASH:
		case TokenType::PERCENT:
		case TokenType::POWER:
		case TokenType::AND:
		case TokenType::OR:
		case TokenType::XOR:
		case TokenType::LSHIFT:
		case TokenType::RSHIFT:
		case TokenType::SLASH:
		case TokenType::EXCLEQ:
		case TokenType::LESS:
		case TokenType::GREATER:
		case TokenType::EQEQ:
		case TokenType::LESSEQ:
		case TokenType::GREATEREQ:
		case TokenType::ANDAND:
		case TokenType::OROR:
		case TokenType::LBRACKET:
		case TokenType::RANGEDOT:
			return num == 2;
		case TokenType::PLUS:
		case TokenType::MINUS:
		case TokenType::STAR:
			return num == 1 || num == 2;
		case TokenType::INCREMENT:
		case TokenType::DECREMENT:
		case TokenType::TILDE:
		case TokenType::EXCL:
			return num == 1;
		case TokenType::LPAR:
			return num >= 1;
	default:
		ASSERT(false, "not operator");
		break;
	}
}

Token::Token()
	: data(""), type(TokenType::NO_TOKEN), errLine(-1) {

}

Token::Token(TokenType type, int errLine) 
	: data(""), type(type), errLine(errLine) {

}

Token::Token(TokenType type, const std::string& data, int errLine) 
	: data(data), type(type), errLine(errLine) {

}

std::string Token::toString() {
	return TOKEN_NAMES[+type] + " \"" + data + "\" line " + std::to_string(errLine + 1);
}

std::string Token::toString(TokenType t) {
	return TOKEN_NAMES[+t];
}
