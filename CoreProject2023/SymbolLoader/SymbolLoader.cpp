#include "SymbolLoader.h"
#include <set>
#include <Utils/ErrorManager.h>
#include <Parser/TypeParser.h>

std::set<TokenType> DEFINABLE_OPERATORS = {
	TokenType::IN, TokenType::IS, TokenType::EQ,
	TokenType::PLUS_EQ, TokenType::MINUS_EQ, TokenType::STAR_EQ,
	TokenType::SLASH_EQ, TokenType::DSLASH_EQ, TokenType::PERCENT_EQ,
	TokenType::POWER_EQ, TokenType::AND_EQ, TokenType::OR_EQ,
	TokenType::XOR_EQ, TokenType::LSHIFT_EQ, TokenType::RSHIFT_EQ,
	TokenType::INCREMENT, TokenType::DECREMENT,

	TokenType::PLUS, TokenType::MINUS, TokenType::STAR,
	TokenType::SLASH, TokenType::DSLASH, TokenType::PERCENT,
	TokenType::POWER,

	TokenType::AND, TokenType::OR, TokenType::XOR,
	TokenType::LSHIFT, TokenType::RSHIFT, TokenType::TILDE,

	TokenType::EXCL, TokenType::EXCLEQ, TokenType::LESS,
	TokenType::GREATER, TokenType::EQEQ, TokenType::LESSEQ,
	TokenType::GREATEREQ,

	TokenType::ANDAND, TokenType::OROR,

	TokenType::LPAR, // ()
	TokenType::LBRACKET, // []

	TokenType::RANGEDOT,
	TokenType::ETCETERA,
};


SymbolLoader::SymbolLoader(std::vector<Token>& toks, const std::string& path)
	: BasicSymbolLoader(toks, path) {

}

void SymbolLoader::loadUse() {
	// TODO: implement here
	skipAssignment();
}

// TODO: implement
void SymbolLoader::loadClass() {
	std::shared_ptr<TypeNode> typeNode = m_symbols.getType(m_pos);

	if (match(TokenType::STRUCT)) {
		std::string name = consume(TokenType::WORD).data;
		std::vector<std::unique_ptr<Type>> fieldTypes;
		consume(TokenType::LBRACE);

		std::vector<Variable> fields;
		std::vector<Function> methods;
		while (!match(TokenType::RBRACE)) {
			readAnnotations();

			if (match(TokenType::DEF)) {
				if (auto prototype = loadMethod(typeNode->qualities, typeNode)) {
					methods.push_back(Function{ std::move(*prototype), nullptr });
				}
			} else {
				Variable field = loadField(typeNode->qualities, typeNode);
				fieldTypes.push_back(field.type->copy());
				fields.push_back(std::move(field));
			}
		}

		std::unique_ptr<StructType> type = std::make_unique<StructType>(std::move(fieldTypes));
		typeNode->llvmType = type->to_llvm();
		typeNode->type = std::move(type);
		typeNode->fields = std::move(fields);
		typeNode->methods = std::move(methods);
	}
}

void SymbolLoader::loadFunction() {
	Function* func = m_symbols.getFunction(m_pos);

	// read function declaration
	match(TokenType::NATIVE);

	std::unique_ptr<Type> returnType;
	if (match(TokenType::TYPE)) { // constructor
		returnType = TypeParser(m_toks, m_pos).consumeType();
	} else if (!match(TokenType::WORD)) {
		if (DEFINABLE_OPERATORS.contains(m_toks[m_pos].type)) { // operator-functions
			if (m_toks[m_pos].type == TokenType::LPAR || m_toks[m_pos].type == TokenType::LBRACKET) {
				m_pos++;
			}

			m_pos++;
		}
	}

	// Arguments
	std::vector<Argument> args;

	consume(TokenType::LPAR);
	if (!match(TokenType::RPAR)) {
		do {
			if (match(TokenType::ETCETERA)) {
				if (peek().type != TokenType::RPAR) {
					ErrorManager::parserError(
						ErrorID::E2105_VA_ARGS_MUST_BE_THE_LAST_ARGUMENT,
						getCurrLine(),
						"incorrect va_args while parsing function " + func->prototype.getName()
					);
				}
			} else {
				std::unique_ptr<Type> type = TypeParser(m_toks, m_pos).consumeType();
				consume(TokenType::WORD);
				args.push_back(Argument{ m_toks[m_pos - 1].data, std::move(type) });
			}
		} while (match(TokenType::COMMA));

		consume(TokenType::RPAR);
	}

	func->prototype.args() = std::move(args);

	if (!returnType) { // not a constructor
		func->prototype.getReturnType() = TypeParser(m_toks, m_pos).parseTypeOrGetNoType();
	} else { // constructor
		func->prototype.getReturnType() = std::move(returnType);
	}

	// skip code
	if (match(TokenType::EQ)) {
		skipAssignment();
	} else if (match(TokenType::LBRACE)) {
		skipCodeInBraces();
	} else {
		consume(TokenType::SEMICOLON);
	}
}

void SymbolLoader::loadVariable() {
	Variable* var = m_symbols.getVariable(m_pos);

	// read variable declaration
	if (!match(TokenType::CONST)) {
		match(TokenType::EXTERN);
	}

	var->type = TypeParser(m_toks, m_pos).consumeType();
	if (var->qualities.getVariableType() == VariableType::CONST) {
		var->type->isConst = true;
	}

	consume(TokenType::WORD);

	// skip code
	if (match(TokenType::EQ)) {
		skipAssignment();
	} else {
		consume(TokenType::SEMICOLON);
	}
}

void SymbolLoader::loadTypeVariable() {
	TypeNode* typeNode = m_symbols.getType(m_pos).get();

	std::string alias = consume(TokenType::WORD).data;
	consume(TokenType::EQ);

	typeNode->type = TypeParser(m_toks, m_pos).consumeType();
	consume(TokenType::SEMICOLON);

	typeNode->llvmType = typeNode->type->to_llvm();
}

std::optional<FunctionPrototype> SymbolLoader::loadMethod(TypeQualities parentQualities, std::shared_ptr<TypeNode> parentType) {
	u64 tokenPos = m_pos;

	FunctionQualities qualities;
	qualities.setIsMethod(true);
	qualities.setSafety(parentQualities.getSafety());
	qualities.setMangling(g_module->getQualities().isManglingOn());

	// read annotations
	for (auto& a : m_annots) {
		if (a[0] == "safe")					qualities.setSafety(Safety::SAFE);
		else if (a[0] == "safe_only")		qualities.setSafety(Safety::SAFE_ONLY);
		else if (a[0] == "unsafe")			qualities.setSafety(Safety::UNSAFE);
		else if (a[0] == "explicit")		qualities.setImplicit(false);
		else if (a[0] == "implicit")		qualities.setImplicit(true);
		else if (a[0] == "ccall")			qualities.setCallingConvention(CallingConvention::CCALL);
		else if (a[0] == "stdcall")			qualities.setCallingConvention(CallingConvention::STDCALL);
		else if (a[0] == "fastcall")		qualities.setCallingConvention(CallingConvention::FASTCALL);
		else if (a[0] == "thiscall")		qualities.setCallingConvention(CallingConvention::THISCALL);
		else if (a[0] == "vectorcall")		qualities.setCallingConvention(CallingConvention::VECTORCALL);
		else if (a[0] == "coldcall")		qualities.setCallingConvention(CallingConvention::COLDCALL);
		else if (a[0] == "tailcall")		qualities.setCallingConvention(CallingConvention::TAILCALL);
		else if (a[0] == "nomangle")		qualities.setMangling(false);
		else if (a[0] == "mangle")			qualities.setMangling(true);
		else if (a[0] == "noreturn")		qualities.setNoReturn(true);
		else if (a[0] == "noexcept")		qualities.setNoExcept(true);
		else ErrorManager::lexerError(
			ErrorID::E1051_UNKNOWN_ANNOTATION,
			getCurrLine(),
			"unknown function annotation: " + a[0]
		);
	}

	if (match(TokenType::PUBLIC)) {
		qualities.setVisibility(Visibility::PUBLIC);
	} else if (match(TokenType::PROTECTED)) {
		qualities.setVisibility(Visibility::DIRECT_IMPORT);
	} else if (match(TokenType::PRIVATE)) {
		qualities.setVisibility(Visibility::PRIVATE);
	}

	if (match(TokenType::STATIC)) {
		qualities.setMethodType(MethodType::STATIC);
	} else if (match(TokenType::VIRTUAL)) {
		qualities.setMethodType(MethodType::VIRTUAL);
	} else if (match(TokenType::ABSTRACT)) {
		qualities.setMethodType(MethodType::ABSTRACT);
	}

	qualities.setNative(match(TokenType::NATIVE));

	std::string alias;
	std::unique_ptr<Type> returnType;
	if (match(TokenType::TYPE)) {
		returnType = TypeParser(m_toks, m_pos).consumeType();
		alias = "type$" + returnType->toString();
		qualities.setIsMethod(false);
		if (qualities.getMethodType() == MethodType::STATIC) {
			ErrorManager::parserError(
				ErrorID::E2107_TYPE_CONSTRUCTOR_IS_STATIC,
				getCurrLine(),
				"type " + returnType->toString()
			);
		}
	} else if (match(TokenType::THIS)) {
		qualities.setFunctionKind(FunctionKind::CONSTRUCTOR);
		alias = "this";
		returnType = TypeNode::genType(parentType);
		qualities.setIsMethod(false);
		if (qualities.getMethodType() == MethodType::STATIC) {
			ErrorManager::parserError(
				ErrorID::E2107_TYPE_CONSTRUCTOR_IS_STATIC,
				getCurrLine(),
				returnType->toString()
			);
		}
	} else if (match(TokenType::WORD)) {
		alias = peek(-1).data;
	} else if (DEFINABLE_OPERATORS.contains(m_toks[m_pos].type)) { // operator-functions
		qualities.setFunctionKind(FunctionKind::OPERATOR);
		alias = m_toks[m_pos].data;
		if (m_toks[m_pos].type == TokenType::LPAR || m_toks[m_pos].type == TokenType::LBRACKET) {
			m_pos++;
		}

		m_pos++;
	}

	// Arguments
	bool isVaArgs = false;
	std::vector<Argument> args;

	if (qualities.getMethodType() != MethodType::STATIC && qualities.getFunctionKind() != FunctionKind::CONSTRUCTOR) {
		args.push_back(Argument("this", std::make_unique<PointerType>(BasicType::REFERENCE, TypeNode::genType(parentType))));
	}

	consume(TokenType::LPAR);
	if (!match(TokenType::RPAR)) {
		do {
			if (match(TokenType::ETCETERA)) {
				if (peek().type != TokenType::RPAR) {
					ErrorManager::parserError(
						ErrorID::E2105_VA_ARGS_MUST_BE_THE_LAST_ARGUMENT,
						getCurrLine(),
						"incorrect va_args while parsing function " + alias
					);
				}

				isVaArgs = true;
			} else {
				std::unique_ptr<Type> type = TypeParser(m_toks, m_pos).consumeType();
				consume(TokenType::WORD);
				args.push_back(Argument{ m_toks[m_pos - 1].data, std::move(type) });
			}
		} while (match(TokenType::COMMA));

		consume(TokenType::RPAR);
	}

	if (qualities.getFunctionKind() == FunctionKind::COMMON) { // not a constructor
		returnType = TypeParser(m_toks, m_pos).parseTypeOrGetNoType();
	}

	// skip code
	if (match(TokenType::EQ)) {
		skipAssignment();
	} else if (match(TokenType::LBRACE)) {
		skipCodeInBraces();
	} else {
		consume(TokenType::SEMICOLON);
	}

	if (qualities.getFunctionKind() == FunctionKind::CONSTRUCTOR) {
		m_symbols.addConstructor(
			qualities.getVisibility(),
			FunctionPrototype(alias, std::move(returnType), std::move(args), qualities, isVaArgs),
			tokenPos
		);

		return std::nullopt;
	} else {
		return { FunctionPrototype(alias, std::move(returnType), std::move(args), qualities, isVaArgs) };
	}
}

Variable SymbolLoader::loadField(TypeQualities parentQualities, std::shared_ptr<TypeNode> parentType) {
	Visibility visibility = Visibility::PUBLIC;
	if (match(TokenType::PUBLIC)) {
		visibility = Visibility::PUBLIC;
	} else if (match(TokenType::PROTECTED)) {
		visibility = Visibility::DIRECT_IMPORT;
	} else if (match(TokenType::PRIVATE)) {
		visibility = Visibility::PRIVATE;
	}

	bool isStatic = match(TokenType::STATIC);
	std::unique_ptr<Type> type = TypeParser(m_toks, m_pos).consumeType();
	std::string fieldName = consume(TokenType::WORD).data;

	if (match(TokenType::EQ)) {
		skipAssignment();
	} else {
		consume(TokenType::SEMICOLON);
	}

	VariableQualities fieldQualities;
	fieldQualities.setSafety(parentType->qualities.getSafety());
	fieldQualities.setVisibility(visibility);
	fieldQualities.setVariableType(VariableType::FIELD);

	return Variable(fieldName, std::move(type), fieldQualities, nullptr);
}
