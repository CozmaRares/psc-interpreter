#include "parser.hpp"

#include <algorithm>
#include <sstream>
#include <utility>

#include "ast.hpp"

template <typename T, typename P>
Result<P> Parser::Ok(P value) {
    return Result<P>::Ok(std::move(value));
}

template <typename T, typename P>
Result<P> Parser::Err(const std::string& reason) {
    return Result<P>::Err(Error(Error::Kind::ParseError, reason, getErrorDetails()));
}

template <typename T, typename P>
Result<P> Parser::Err(Error error) {
    return Result<P>::Err(error);
}

std::string Parser::getErrorDetails() const {
    std::ostringstream oss;
    oss << "token " << m_currentToken->kind;

    if (!m_currentToken->value.empty())
        oss << " (" << m_currentToken->value << ")";

    return oss.str();
}

std::string toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

// errors
#define RETURN_IF_ERROR(result) \
    if (result.isError())       \
        return result;

#define PROPAGATE_IF_ERROR_RET(result, ret) \
    if (result.isError())                   \
        return Err<ret>(result.getError());

#define PROPAGATE_IF_ERROR(result) \
    if (result.isError())          \
        return Err<AST::Node>(result.getError());

// generic checks
#define ADVANCE_IF_TOKEN_RET(kindCheck, ret, chars)                           \
    if (m_currentToken->kind != Token::Kind::kindCheck)                       \
        return Err<ret>("Expected " + toLower(#kindCheck) + " (" #chars ")"); \
    advance();

#define ADVANCE_IF_TOKEN(kindCheck, chars)                                          \
    if (m_currentToken->kind != Token::Kind::kindCheck)                             \
        return Err<AST::Node>("Expected " + toLower(#kindCheck) + " (" #chars ")"); \
    advance();

#define STORE_AND_ADVANCE_IF_TOKEN_RET(kindCheck, identifier, ret, msg) \
    if (m_currentToken->kind != Token::Kind::kindCheck)                 \
        return Err<ret>(msg);                                           \
    std::string identifier = m_currentToken->value;                     \
    advance();

#define STORE_AND_ADVANCE_IF_TOKEN(kindCheck, identifier, msg) \
    if (m_currentToken->kind != Token::Kind::kindCheck)        \
        return Err<AST::Node>(msg);                            \
    std::string identifier = m_currentToken->value;            \
    advance();

// specialized checks
#define ADVANCE_IF_KEYWORD_RET(kindCheck, ret)                             \
    if (m_currentToken->kind != Token::Kind::kindCheck)                    \
        return Err<ret>("Expected '" + toLower(#kindCheck) + "' keyword"); \
    advance();

#define ADVANCE_IF_KEYWORD(kindCheck)                                            \
    if (m_currentToken->kind != Token::Kind::kindCheck)                          \
        return Err<AST::Node>("Expected '" + toLower(#kindCheck) + "' keyword"); \
    advance();

#define STORE_AND_ADVANCE_IF_IDENTIFIER_RET(identifier, ret) \
    if (m_currentToken->kind != Token::Kind::Identifier)     \
        return Err<ret>("Expected identifier");              \
    std::string identifier = m_currentToken->value;          \
    advance();

#define STORE_AND_ADVANCE_IF_IDENTIFIER(identifier)      \
    if (m_currentToken->kind != Token::Kind::Identifier) \
        return Err<AST::Node>("Expected identifier");    \
    std::string identifier = m_currentToken->value;      \
    advance();

void Parser::advance() {
    m_currentIndex++;
    m_currentToken =
        m_currentIndex < m_tokens.size()
            ? m_tokens[m_currentIndex].get()
            : nullptr;
}

// Z ::= EXPRESSION EXPRESSIONS
Result<std::unique_ptr<AST::Node>> Parser::parseZ() {
    Result<std::unique_ptr<AST::Node>> expressionResult = parseExpression();
    RETURN_IF_ERROR(expressionResult);

    Result<std::unique_ptr<AST::Expressions>> expressionsResult = parseExpressions();
    PROPAGATE_IF_ERROR(expressionResult);

    std::unique_ptr<AST::Node> expression         = expressionResult.getValue();
    std::unique_ptr<AST::Expressions> expressions = expressionsResult.getValue();

    std::unique_ptr<AST::Expressions> allExpressions = AST::Expressions::FromNode(std::move(expression));
    allExpressions->extend(std::move(expressions));

    std::unique_ptr<AST::Node> ret = std::move(allExpressions);

    return Ok<AST::Node>(std::move(ret));
}

// EXPRESSIONS ::= ('\n' EXPRESSION)*
Result<std::unique_ptr<AST::Expressions>> Parser::parseExpressions() {
    std::vector<std::unique_ptr<AST::Node>> expressions;

    while (m_currentToken->kind == Token::Kind::Endline) {
        advance();

        auto expressionResult = parseExpression();
        PROPAGATE_IF_ERROR_RET(expressionResult, AST::Expressions);

        auto expression = expressionResult.getValue();
        expressions.push_back(std::move(expression));
    }

    return Ok<AST::Node>(std::make_unique<AST::Expressions>(std::move(expressions)));
}

/*
EXPRESSION ::= EXPRESSION_IF
             | EXPRESSION_FOR
             | EXPRESSION_WHILE
             | EXPRESSION_DO_UNTIL
             | EXPRESSION_CONTINUE
             | EXPRESSION_BREAK
             | EXPRESSION_TRYCATCH
             | EXPRESSION_THROW
             | EXPRESSION_FUNCTION
             | EXPRESSION_RETURN
             | EXPRESSION_INCLUDE
             | EXPRESSION_RUN
             | EXPRESSION_READ
             | EXPRESSION_PRINT
             | MEMORY_WRITE
             | OPERATION
*/
Result<std::unique_ptr<AST::Node>> Parser::parseExpression() {
    if (m_currentToken->kind == Token::Kind::If)
        return parseIf();

    if (m_currentToken->kind == Token::Kind::For)
        return parseFor();

    if (m_currentToken->kind == Token::Kind::While)
        return parseWhile();

    if (m_currentToken->kind == Token::Kind::Do)
        return parseDoUntil();

    if (m_currentToken->kind == Token::Kind::Continue)
        return parseContinue();

    if (m_currentToken->kind == Token::Kind::Break)
        return parseBreak();

    if (m_currentToken->kind == Token::Kind::Try)
        return parseTryCatch();

    if (m_currentToken->kind == Token::Kind::Throw)
        return parseThrow();

    if (m_currentToken->kind == Token::Kind::Function)
        return parseFunction();

    if (m_currentToken->kind == Token::Kind::Return)
        return parseReturn();

    if (m_currentToken->kind == Token::Kind::Include)
        return parseInclude();

    if (m_currentToken->kind == Token::Kind::Run)
        return parseRun();

    if (m_currentToken->kind == Token::Kind::Read)
        return parseRead();

    if (m_currentToken->kind == Token::Kind::Print)
        return parsePrint();

    if (m_currentToken->kind == Token::Kind::Let)
        return parseAssignment();

    return parseOperation();
}

// EXPRESSION_IF ::= <if> EXPRESSION <then> EXPRESSIONS (<else> EXPRESSIONS)? <end>
Result<std::unique_ptr<AST::Node>> Parser::parseIf() {
    ADVANCE_IF_KEYWORD(If);

    Result<std::unique_ptr<AST::Node>> conditionResult = parseExpression();
    PROPAGATE_IF_ERROR(conditionResult);

    ADVANCE_IF_KEYWORD(Then);

    Result<std::unique_ptr<AST::Expressions>> trueBodyResult = parseExpressions();
    PROPAGATE_IF_ERROR(trueBodyResult);

    std::optional<std::unique_ptr<AST::Expressions>> falseBody = std::nullopt;

    if (m_currentToken->kind == Token::Kind::Else) {
        advance();

        Result<std::unique_ptr<AST::Expressions>> falseBodyResult = parseExpressions();
        PROPAGATE_IF_ERROR(falseBodyResult);

        falseBody = falseBodyResult.getValue();
    }

    ADVANCE_IF_KEYWORD(End);

    std::unique_ptr<AST::Node> ret =
        std::make_unique<AST::IfExpression>(
            conditionResult.getValue(),
            trueBodyResult.getValue(),
            std::move(falseBody));
    return Ok<AST::Node>(std::move(ret));
}

// EXPRESSION_FOR ::= <for> identifier '<-' EXPRESSION ',' EXPRESSION (',' EXPRESSION)? <execute> EXPRESSIONS <end>
Result<std::unique_ptr<AST::Node>> Parser::parseFor() {
    ADVANCE_IF_KEYWORD(For);

    STORE_AND_ADVANCE_IF_IDENTIFIER(ident);

    ADVANCE_IF_TOKEN(Assignment, "<-");

    Result<std::unique_ptr<AST::Node>> startValueResult = parseExpression();
    RETURN_IF_ERROR(startValueResult);

    ADVANCE_IF_TOKEN(Comma, ",");

    Result<std::unique_ptr<AST::Node>> stopValueResult = parseExpression();
    RETURN_IF_ERROR(stopValueResult);

    std::optional<std::unique_ptr<AST::Node>> skipValue = std::nullopt;

    if (m_currentToken->kind == Token::Kind::Comma) {
        advance();

        Result<std::unique_ptr<AST::Node>> skipValueResult = parseExpression();
        RETURN_IF_ERROR(skipValueResult);

        skipValue = skipValueResult.getValue();
    }

    ADVANCE_IF_KEYWORD(Execute);

    Result<std::unique_ptr<AST::Expressions>> bodyResult = parseExpressions();
    PROPAGATE_IF_ERROR(bodyResult);

    ADVANCE_IF_KEYWORD(End);

    std::unique_ptr<AST::Node> ret = std::make_unique<AST::ForExpression>(
        ident,
        startValueResult.getValue(),
        stopValueResult.getValue(),
        std::move(skipValue),
        bodyResult.getValue());
    return Ok<AST::Node>(std::move(ret));
}

// EXPRESSION_WHILE ::= <while> EXPRESSION <execute> EXPRESSIONS <end>
Result<std::unique_ptr<AST::Node>> Parser::parseWhile() {
    ADVANCE_IF_KEYWORD(While);

    Result<std::unique_ptr<AST::Node>> conditionResult = parseExpression();
    RETURN_IF_ERROR(conditionResult);

    ADVANCE_IF_KEYWORD(Execute);

    Result<std::unique_ptr<AST::Expressions>> body = parseExpressions();
    PROPAGATE_IF_ERROR(body);

    ADVANCE_IF_KEYWORD(End);

    std::unique_ptr<AST::Node> ret =
        std::make_unique<AST::WhileExpression>(
            conditionResult.getValue(),
            body.getValue());
    return Ok<AST::Node>(std::move(ret));
}

// EXPRESSION_DO_UNTIL ::= <do> EXPRESSIONS <until> EXPRESSION
Result<std::unique_ptr<AST::Node>> Parser::parseDoUntil() {
    ADVANCE_IF_KEYWORD(Do);

    Result<std::unique_ptr<AST::Expressions>> bodyResult = parseExpressions();
    PROPAGATE_IF_ERROR(bodyResult);

    ADVANCE_IF_KEYWORD(Until);

    Result<std::unique_ptr<AST::Node>> conditionResult = parseExpression();
    RETURN_IF_ERROR(conditionResult);

    std::unique_ptr<AST::Node> ret =
        std::make_unique<AST::DoUntilExpression>(
            bodyResult.getValue(),
            conditionResult.getValue());
    return Ok<AST::Node>(std::move(ret));
}

// EXPRESSION_CONTINUE ::= <continue>
Result<std::unique_ptr<AST::Node>> Parser::parseContinue() {
    ADVANCE_IF_KEYWORD(Continue);

    std::unique_ptr<AST::Node> ret =
        std::make_unique<AST::ContinueExpression>();
    return Ok<AST::Node>(std::move(ret));
}

// EXPRESSION_BREAK ::= <break>
Result<std::unique_ptr<AST::Node>> Parser::parseBreak() {
    ADVANCE_IF_KEYWORD(Break);

    std::unique_ptr<AST::Node> ret =
        std::make_unique<AST::BreakExpression>();
    return Ok<AST::Node>(std::move(ret));
}

// EXPRESSION_TRYCATCH ::= <try> EXPRESSIONS <catch> identifier <then> EXPRESSIONS <end>
Result<std::unique_ptr<AST::Node>> Parser::parseTryCatch() {
    ADVANCE_IF_KEYWORD(Try);

    Result<std::unique_ptr<AST::Expressions>> tryBodyResult = parseExpressions();
    PROPAGATE_IF_ERROR(tryBodyResult);

    ADVANCE_IF_KEYWORD(Catch);

    STORE_AND_ADVANCE_IF_IDENTIFIER(errorIdent);

    ADVANCE_IF_KEYWORD(Then);

    Result<std::unique_ptr<AST::Expressions>> catchBodyResult = parseExpressions();
    PROPAGATE_IF_ERROR(catchBodyResult);

    ADVANCE_IF_KEYWORD(End);

    std::unique_ptr<AST::Node> ret =
        std::make_unique<AST::TryCatchExpression>(
            tryBodyResult.getValue(),
            errorIdent,
            catchBodyResult.getValue());
    return Ok<AST::Node>(std::move(ret));
}

// EXPRESSION_THROW ::= <throw> EXPRESSION
Result<std::unique_ptr<AST::Node>> Parser::parseThrow() {
    ADVANCE_IF_KEYWORD(Throw);

    Result<std::unique_ptr<AST::Node>> valueResult = parseExpression();
    RETURN_IF_ERROR(valueResult);

    std::unique_ptr<AST::Node> ret =
        std::make_unique<AST::ThrowExpression>(
            valueResult.getValue());
    return Ok<AST::Node>(std::move(ret));
}

// EXPRESSION_FUNCTION ::= <function> identifier '(' (identifier (',' identifier)*)? ')' ':' EXPRESSIONS <end>
Result<std::unique_ptr<AST::Node>> Parser::parseFunction() {
    ADVANCE_IF_KEYWORD(Function);

    STORE_AND_ADVANCE_IF_IDENTIFIER(fnIdent);

    ADVANCE_IF_TOKEN(ParenLeft, "(");

    std::vector<std::string> args;

    if (m_currentToken->kind == Token::Kind::Identifier) {
        args.push_back(m_currentToken->value);
        advance();

        while (m_currentToken->kind == Token::Kind::Comma) {
            advance();

            if (m_currentToken->kind != Token::Kind::Identifier)
                return Err<AST::Node>("Expected identifier");

            args.push_back(m_currentToken->value);
            advance();
        }
    }

    ADVANCE_IF_TOKEN(ParenRight, ")");

    ADVANCE_IF_TOKEN(Colon, ":");

    Result<std::unique_ptr<AST::Expressions>> bodyResult = parseExpressions();
    PROPAGATE_IF_ERROR(bodyResult);

    ADVANCE_IF_KEYWORD(End);

    std::unique_ptr<AST::Node> ret =
        std::make_unique<AST::FunctionDefinitionExpression>(
            fnIdent, args, bodyResult.getValue());
    return Ok<AST::Node>(std::move(ret));
}

// EXPRESSION_RETURN ::= <return> EXPRESSION
Result<std::unique_ptr<AST::Node>> Parser::parseReturn() {
    ADVANCE_IF_KEYWORD(Return);

    Result<std::unique_ptr<AST::Node>> valueResult = parseExpression();
    RETURN_IF_ERROR(valueResult);

    std::unique_ptr<AST::Node> ret =
        std::make_unique<AST::ReturnExpression>(
            valueResult.getValue());
    return Ok<AST::Node>(std::move(ret));
}

// EXPRESSION_INCLUDE ::= <include> string
Result<std::unique_ptr<AST::Node>> Parser::parseInclude() {
    ADVANCE_IF_KEYWORD(Include);

    STORE_AND_ADVANCE_IF_TOKEN(String, path, "Expected string");

    std::unique_ptr<AST::Node> ret =
        std::make_unique<AST::IncludeExpression>(path);
    return Ok<AST::Node>(std::move(ret));
}

// EXPRESSION_RUN ::= <run> string
Result<std::unique_ptr<AST::Node>> Parser::parseRun() {
    ADVANCE_IF_KEYWORD(Run);

    STORE_AND_ADVANCE_IF_TOKEN(String, path, "Expected string");

    std::unique_ptr<AST::Node> ret =
        std::make_unique<AST::RunExpression>(path);
    return Ok<AST::Node>(std::move(ret));
}

// EXPRESSION_READ ::= <read> ('<' identifier '>')? identifier (',' identifier)*
Result<std::unique_ptr<AST::Node>> Parser::parseRead() {
    ADVANCE_IF_KEYWORD(Read);

    std::optional<std::string> fileIdentifier = std::nullopt;

    if (m_currentToken->kind == Token::Kind::Less) {
        advance();

        if (m_currentToken->kind != Token::Kind::Identifier)
            return Err<AST::Node>("Expected identifier");

        fileIdentifier = m_currentToken->value;
        advance();
    }

    std::vector<std::string> identifiers;

    if (m_currentToken->kind != Token::Kind::Identifier)
        return Err<AST::Node>("Expected identifier");

    identifiers.push_back(m_currentToken->value);

    while (m_currentToken->kind == Token::Kind::Comma) {
        advance();

        if (m_currentToken->kind != Token::Kind::Identifier)
            return Err<AST::Node>("Expected identifier");

        identifiers.push_back(m_currentToken->value);
        advance();
    }

    std::unique_ptr<AST::Node> ret =
        std::make_unique<AST::ReadExpression>(
            fileIdentifier, identifiers);
    return Ok<AST::Node>(std::move(ret));
}

// EXPRESSION_PRINT ::= <print> ('<' identifier '>')? EXPRESSION (',' EXPRESSION)*
Result<std::unique_ptr<AST::Node>> Parser::parsePrint() {
    ADVANCE_IF_KEYWORD(Print);

    std::optional<std::string> fileIdentifier = std::nullopt;

    if (m_currentToken->kind == Token::Kind::Less) {
        advance();

        if (m_currentToken->kind != Token::Kind::Identifier)
            return Err<AST::Node>("Expected identifier");

        fileIdentifier = m_currentToken->value;
        advance();
    }

    std::vector<std::unique_ptr<AST::Node>> expressions;

    Result<std::unique_ptr<AST::Node>> expressionResult1 = parseExpression();
    RETURN_IF_ERROR(expressionResult1);

    expressions.push_back(expressionResult1.getValue());

    while (m_currentToken->kind == Token::Kind::Comma) {
        advance();

        Result<std::unique_ptr<AST::Node>> expressionResult = parseExpression();
        RETURN_IF_ERROR(expressionResult);

        expressions.push_back(expressionResult.getValue());
    }

    std::unique_ptr<AST::Node> ret =
        std::make_unique<AST::PrintExpression>(
            fileIdentifier, std::move(expressions));
    return Ok<AST::Node>(std::move(ret));
}

// MEMORY_WRITE ::= <let> identifier INDEX_ACCESS* '<-' EXPRESSION
Result<std::unique_ptr<AST::Node>> Parser::parseAssignment() {
    ADVANCE_IF_KEYWORD(Let);

    STORE_AND_ADVANCE_IF_IDENTIFIER(ident);

    std::vector<std::unique_ptr<AST::Node>> indexes;

    while (m_currentToken->kind == Token::Kind::BracketLeft) {
        Result<std::unique_ptr<AST::Node>> indexesResult = parseIndexAccess();
        RETURN_IF_ERROR(indexesResult);
        indexes.push_back(indexesResult.getValue());
    }

    ADVANCE_IF_TOKEN(Assignment, "<-");

    Result<std::unique_ptr<AST::Node>> valueResult = parseExpression();
    RETURN_IF_ERROR(valueResult);

    std::unique_ptr<AST::Node> ret =
        std::make_unique<AST::Assignment>(
            ident,
            std::move(indexes),
            valueResult.getValue());
    return Ok<AST::Node>(std::move(ret));
}

// INDEX_ACCESS ::= '[' EXPRESSION ']'
Result<std::unique_ptr<AST::Node>> Parser::parseIndexAccess() {
    ADVANCE_IF_TOKEN(BracketLeft, "[");

    Result<std::unique_ptr<AST::Node>> expressionResult = parseExpression();
    RETURN_IF_ERROR(expressionResult);

    ADVANCE_IF_TOKEN(BracketRight, "]");

    return expressionResult;
}

// OPERATION ::= COMPARISON ( (<and> | <or>) COMPARISON )*
Result<std::unique_ptr<AST::Node>> Parser::parseOperation() {
    Result<std::unique_ptr<AST::Node>> leftResult = parseComparison();
    RETURN_IF_ERROR(leftResult);

    std::unique_ptr<AST::Node> acc = leftResult.getValue();

    while (
        m_currentToken->kind == Token::Kind::And ||
        m_currentToken->kind == Token::Kind::Or) {
        Token::Kind opKind = m_currentToken->kind;
        advance();

        Result<std::unique_ptr<AST::Node>> rightResult = parseComparison();
        RETURN_IF_ERROR(rightResult);

        acc = std::make_unique<AST::BinaryOperation>(
            std::move(acc),
            opKind,
            std::move(rightResult.getValue()));
    }

    return Ok<AST::Node>(std::move(acc));
}

// COMPARISON ::= ARITH ('=' | '<' | '<=' | '>' | '>=' | '!=') ARITH
Result<std::unique_ptr<AST::Node>> Parser::parseComparison() {
    Result<std::unique_ptr<AST::Node>> leftResult = parseArith();
    RETURN_IF_ERROR(leftResult);

    std::unique_ptr<AST::Node> acc = leftResult.getValue();

    while (
        m_currentToken->kind == Token::Kind::Equals ||
        m_currentToken->kind == Token::Kind::Less ||
        m_currentToken->kind == Token::Kind::LessEqual ||
        m_currentToken->kind == Token::Kind::Greater ||
        m_currentToken->kind == Token::Kind::GreaterEqual ||
        m_currentToken->kind == Token::Kind::Different) {
        Token::Kind opKind = m_currentToken->kind;
        advance();

        Result<std::unique_ptr<AST::Node>> rightResult = parseArith();
        RETURN_IF_ERROR(rightResult);

        acc = std::make_unique<AST::BinaryOperation>(
            std::move(acc),
            opKind,
            std::move(rightResult.getValue()));
    }

    return Ok<AST::Node>(std::move(acc));
}

// ARITH ::= TERM ('+' | '-') TERM
Result<std::unique_ptr<AST::Node>> Parser::parseArith() {
    Result<std::unique_ptr<AST::Node>> leftResult = parseTerm();
    RETURN_IF_ERROR(leftResult);

    std::unique_ptr<AST::Node> acc = leftResult.getValue();

    while (
        m_currentToken->kind == Token::Kind::Plus ||
        m_currentToken->kind == Token::Kind::Minus) {
        Token::Kind opKind = m_currentToken->kind;
        advance();

        Result<std::unique_ptr<AST::Node>> rightResult = parseTerm();
        RETURN_IF_ERROR(rightResult);

        acc = std::make_unique<AST::BinaryOperation>(
            std::move(acc),
            opKind,
            std::move(rightResult.getValue()));
    }

    return Ok<AST::Node>(std::move(acc));
}

// TERM ::= FACTOR ('*' | '/' | '%') FACTOR
Result<std::unique_ptr<AST::Node>> Parser::parseTerm() {
    Result<std::unique_ptr<AST::Node>> leftResult = parseFactor();
    RETURN_IF_ERROR(leftResult);

    std::unique_ptr<AST::Node> acc = leftResult.getValue();

    while (
        m_currentToken->kind == Token::Kind::Multiply ||
        m_currentToken->kind == Token::Kind::Divide ||
        m_currentToken->kind == Token::Kind::Modulo) {
        Token::Kind opKind = m_currentToken->kind;
        advance();

        Result<std::unique_ptr<AST::Node>> rightResult = parseFactor();
        RETURN_IF_ERROR(rightResult);

        acc = std::make_unique<AST::BinaryOperation>(
            std::move(acc),
            opKind,
            std::move(rightResult.getValue()));
    }

    return Ok<AST::Node>(std::move(acc));
}

using VecNode = std::vector<std::unique_ptr<AST::Node>>;

/*
FACTOR ::= BASE FN_CALL
         | BASE INDEX_ACCESS*
*/
Result<std::unique_ptr<AST::Node>> Parser::parseFactor() {
    Result<std::unique_ptr<AST::Node>> baseResult = parseBase();
    RETURN_IF_ERROR(baseResult);

    std::unique_ptr<AST::Node> acc = baseResult.getValue();

    if (m_currentToken->kind == Token::Kind::ParenLeft) {
        Result<VecNode> fnCallResult = parseFnCall();
        PROPAGATE_IF_ERROR(fnCallResult);

        acc =
            std::make_unique<AST::FunctionCallExpression>(
                std::move(acc),
                std::move(fnCallResult.getValue()));
    }

    while (m_currentToken->kind == Token::Kind::BracketLeft) {
        Result<std::unique_ptr<AST::Node>> indexResult = parseIndexAccess();
        RETURN_IF_ERROR(indexResult);

        acc = std::make_unique<AST::IndexAccess>(
            std::move(acc),
            std::move(indexResult.getValue()));
    }

    return Ok<AST::Node>(std::move(acc));
}

// FN_CALL ::= '(' ( EXPRESSION (',' EXPRESSION)* )? ')'
Result<VecNode> Parser::parseFnCall() {
    if (m_currentToken->kind != Token ::Kind ::ParenLeft)
        return Result<VecNode>::Err(Error(
            Error::Kind::ParseError,
            "Expected " + toLower("ParenLeft") + " ( \"(\")",
            getErrorDetails()));
    advance();

    VecNode args;

    Result<std::unique_ptr<AST::Node>> expressionResult1 = parseExpression();
    if (expressionResult1.isError())
        return Result<VecNode>::Err(expressionResult1.getError());

    args.push_back(expressionResult1.getValue());

    while (m_currentToken->kind == Token::Kind::Comma) {
        advance();

        Result<std::unique_ptr<AST::Node>> expressionResult = parseExpression();
        if (expressionResult.isError())
            return Result<VecNode>::Err(expressionResult.getError());

        args.push_back(expressionResult.getValue());
    }

    if (m_currentToken->kind != Token ::Kind ::ParenRight)
        return Result<VecNode>::Err(Error(
            Error::Kind::ParseError,
            "Expected " + toLower("ParenRight") + " ( \")\" )",
            getErrorDetails()));
    advance();

    return Result<VecNode>::Ok(std::move(args));
}

/*
BASE ::= number
       | char
       | string
       | identifier
       | ARRAY
       | DICTIONARY
       | '(' EXPRESSION ')'
       | ('+' | '-') BASE
*/
Result<std::unique_ptr<AST::Node>> Parser::parseBase() {
    std::unique_ptr<AST::Node> n(nullptr);

    switch (m_currentToken->kind) {
        case Token::Kind::Number:
            n = std::make_unique<AST::Number>(std::stod(m_currentToken->value));
            advance();
            break;

        case Token::Kind::Char:
            n = std::make_unique<AST::Char>(m_currentToken->value[0]);
            advance();
            break;

        case Token::Kind::String:
            n = std::make_unique<AST::String>(m_currentToken->value);
            advance();
            break;

        case Token::Kind::Identifier:
            n = std::make_unique<AST::VariableAccess>(m_currentToken->value);
            advance();
            break;

        case Token::Kind::ParenLeft:
            return parseArray();

        case Token::Kind::CurlyLeft:
            return parseDictionary();

        case Token::Kind::Plus:
        case Token::Kind::Minus:
            return parseUnary();

        default:
            return Err<AST::Node>("Expected one of: number, char, string, identifier, {, [, +, -");
    }

    return Ok<AST::Node>(std::move(n));
}

// ARRAY ::= '[' (EXPRESSION (',' EXPRESSION)*)? ']'
Result<std::unique_ptr<AST::Node>> Parser::parseArray() {
    ADVANCE_IF_TOKEN(BracketLeft, "[");

    VecNode elements;

    Result<std::unique_ptr<AST::Node>> expressionResult1 = parseExpression();
    RETURN_IF_ERROR(expressionResult1);

    elements.push_back(expressionResult1.getValue());

    while (m_currentToken->kind == Token::Kind::Comma) {
        advance();

        Result<std::unique_ptr<AST::Node>> expressionResult = parseExpression();
        RETURN_IF_ERROR(expressionResult);

        elements.push_back(expressionResult.getValue());
    }

    ADVANCE_IF_TOKEN(ParenRight, ")");

    std::unique_ptr<AST::Node> ret =
        std::make_unique<AST::Array>(std::move(elements));
    return Ok<AST::Node>(std::move(ret));
}

// DICTIONARY ::= '{' (EXPRESSION ':' EXPRESSION (',' EXPRESSION ':' EXPRESSION)*)? '}'
Result<std::unique_ptr<AST::Node>> Parser::parseDictionary() {
    ADVANCE_IF_TOKEN(CurlyLeft, "{");

    using VecPairs = std::vector<std::pair<std::unique_ptr<AST::Node>, std::unique_ptr<AST::Node>>>;

    VecPairs pairs;

    Result<std::unique_ptr<AST::Node>> keyResult1 = parseExpression();
    RETURN_IF_ERROR(keyResult1);

    ADVANCE_IF_TOKEN(Comma, ",");

    Result<std::unique_ptr<AST::Node>> valueResult1 = parseExpression();
    RETURN_IF_ERROR(valueResult1);

    pairs.push_back({ keyResult1.getValue(), valueResult1.getValue() });

    while (m_currentToken->kind == Token::Kind::Comma) {
        advance();

        Result<std::unique_ptr<AST::Node>> keyResult = parseExpression();
        RETURN_IF_ERROR(keyResult);

        ADVANCE_IF_TOKEN(Comma, ",");

        Result<std::unique_ptr<AST::Node>> valueResult = parseExpression();
        RETURN_IF_ERROR(valueResult);

        pairs.push_back({ keyResult.getValue(), valueResult.getValue() });
    }

    ADVANCE_IF_TOKEN(ParenRight, ")");

    std::unique_ptr<AST::Node> ret =
        std::make_unique<AST::Dictionary>(std::move(pairs));
    return Ok<AST::Node>(std::move(ret));
}

// UNARY ::= ('+' | '-') BASE
Result<std::unique_ptr<AST::Node>> Parser::parseUnary() {
    if (m_currentToken->kind != Token::Kind::Plus &&
        m_currentToken->kind != Token::Kind::Minus)
        return Err<AST::Node>("Expected one of: plus, minus");

    Token::Kind opKind = m_currentToken->kind;
    advance();

    auto base = parseBase();
    RETURN_IF_ERROR(base);

    std::unique_ptr<AST::Node> ret =
        std::make_unique<AST::UnaryOperation>(opKind, base.getValue());
    return Ok<AST::Node>(std::move(ret));
}

Result<std::unique_ptr<AST::Node>> Parser::Parse(std::vector<std::unique_ptr<Token>>&& tokens) {
    Parser parser(std::move(tokens));
    return parser.parseZ();
}
