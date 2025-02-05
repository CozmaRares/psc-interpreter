#pragma once

#include <memory>
#include <vector>

#include "../lex/tokens.hpp"
#include "../utils.hpp"
#include "ast.hpp"

class Parser {
    std::vector<std::unique_ptr<Token>> m_tokens;
    Token* m_currentToken;
    size_t m_currentIndex;

    explicit Parser(std::vector<std::unique_ptr<Token>>&& tokens)
        : m_tokens(std::move(tokens)) {
        m_currentIndex = 0;
        m_currentToken =
            m_tokens.size() > 0
                ? m_tokens[0].get()
                : nullptr;
    }

    void advance();

    Result<std::unique_ptr<AST::Node>> parseZ();
    Result<std::unique_ptr<AST::Expressions>> parseExpressions();
    Result<std::unique_ptr<AST::Node>> parseExpression();
    Result<std::unique_ptr<AST::Node>> parseIf();
    Result<std::unique_ptr<AST::Node>> parseFor();
    Result<std::unique_ptr<AST::Node>> parseWhile();
    Result<std::unique_ptr<AST::Node>> parseDoUntil();
    Result<std::unique_ptr<AST::Node>> parseContinue();
    Result<std::unique_ptr<AST::Node>> parseBreak();
    Result<std::unique_ptr<AST::Node>> parseTryCatch();
    Result<std::unique_ptr<AST::Node>> parseThrow();
    Result<std::unique_ptr<AST::Node>> parseFunction();
    Result<std::unique_ptr<AST::Node>> parseReturn();
    Result<std::unique_ptr<AST::Node>> parseInclude();
    Result<std::unique_ptr<AST::Node>> parseRun();
    Result<std::unique_ptr<AST::Node>> parseRead();
    Result<std::unique_ptr<AST::Node>> parsePrint();
    Result<std::unique_ptr<AST::Node>> parseAssignment();
    Result<std::unique_ptr<AST::Node>> parseIndexAccess();
    Result<std::vector<std::unique_ptr<AST::Node>>> parseFnCall();
    Result<std::unique_ptr<AST::Node>> parseOperation();
    Result<std::unique_ptr<AST::Node>> parseComparison();
    Result<std::unique_ptr<AST::Node>> parseArith();
    Result<std::unique_ptr<AST::Node>> parseTerm();
    Result<std::unique_ptr<AST::Node>> parseFactor();
    Result<std::unique_ptr<AST::Node>> parseBase();
    Result<std::unique_ptr<AST::Node>> parseArray();
    Result<std::unique_ptr<AST::Node>> parseDictionary();
    Result<std::unique_ptr<AST::Node>> parseUnary();

    template <typename T, typename P = std::unique_ptr<T>>
    static Result<P> Ok(P value);

    template <typename T, typename P = std::unique_ptr<T>>
    Result<P> Err(const std::string& reason);

    template <typename T, typename P = std::unique_ptr<T>>
    static Result<P> Err(Error error);

    std::string getErrorDetails() const;

  public:
    static Result<std::unique_ptr<AST::Node>> Parse(std::vector<std::unique_ptr<Token>>&& tokens);
};
