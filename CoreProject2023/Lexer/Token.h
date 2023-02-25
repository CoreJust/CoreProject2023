#pragma once
#include <string>
#include <Utils/Defs.h>

enum class TokenType : u8 {
	// literals
	NUMBERI8 = 0,
	NUMBERU8,
	NUMBERI16,
	NUMBERU16,
	NUMBERI32,
	NUMBERU32,
	NUMBERI64,
	NUMBERU64,

	NUMBERF32,
	NUMBERF64,

	LETTER8,
	LETTER16,
	LETTER32,
	TEXT8,
	TEXT16,
	TEXT32,
	FORMAT_TEXT8,
	FORMAT_TEXT16,
	FORMAT_TEXT32,
	FORMAT_STRING_END,

	WORD, // identifier

	// keywords
	IMPORT,
	USE,

	DEF,
	NATIVE,
	RETURN,

	CLASS,
	INTERFACE,
	STRUCT,
	ENUM,
	UNION,
	TUPLE,

	PUBLIC,
	PRIVATE,
	PROTECTED,
	ABSTRACT,
	VIRTUAL,
	THIS,

	CONST,
	VAR,
	EXTERN,
	STATIC,

	UNSAFE,
	CT,

	IF,
	ELSE,
	ELIF,

	SWITCH,
	MATCH,
	CASE,
	DEFAULT,

	FOR,
	WHILE,
	DO,

	BREAK,
	CONTINUE,

	TRUE,
	FALSE,
	NULLPTR, // null

	NEW,
	DELETE,
	IN,
	IS,
	AS,
	TYPEOF,

	MOVE,

	TYPE,

	BOOL,
	C8,
	C16,
	C32,
	STR8,
	STR16,
	STR32,
	I8,
	I16,
	I32,
	I64,
	U8,
	U16,
	U32,
	U64,
	F32,
	F64,
	FUNC,

	// operators
	EQ, // =

	PLUS_EQ, // +=
	MINUS_EQ, // -=
	STAR_EQ, // *=
	SLASH_EQ, // /=
	DSLASH_EQ, // //=
	PERCENT_EQ, // %=
	POWER_EQ, // **=
	AND_EQ, // &=
	OR_EQ, // |=
	XOR_EQ, // ^=
	LSHIFT_EQ, // <<=
	RSHIFT_EQ, // >>=

	INCREMENT, // ++
	DECREMENT, // --

	PLUS, // +
	MINUS, // -
	STAR, // *
	SLASH, // /
	DSLASH, // //
	PERCENT, // %
	POWER, // **

	AND, // &
	OR, // |
	XOR, // ^
	LSHIFT, // <<
	RSHIFT, // >>
	TILDE, // ~

	EXCL, // !
	EXCLEQ, // !=
	LESS, // <
	GREATER, // >
	EQEQ, // ==
	LESSEQ, // <=
	GREATEREQ, // >=

	ANDAND, // &&
	OROR, // ||

	LPAR, // (
	RPAR, // )
	LBRACE, // {
	RBRACE, // }
	LBRACKET, // [
	RBRACKET, // ]

	COMMA, // ,
	DOT, // .
	RANGEDOT, // ..
	ETCETERA, // ...
	QUESTION, // ?
	COLON, // :
	SEMICOLON, // ;
	AT, // @

	NO_TOKEN
};

constexpr u8 operator+(TokenType t) {
	return u8(t);
}

// Is the number of arguments possible for this operator
bool isPossibleNumArgumentsOfOperator(TokenType op, u32 num);

struct Token {
	std::string data;
	int errLine; // the line where the token is located in the text
	TokenType type;

	Token();
	Token(TokenType type, int errLine);
	Token(TokenType type, const std::string& data, int errLine);

	std::string toString() const;
	static std::string toString(TokenType t);
};