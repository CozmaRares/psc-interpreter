#pragma once

#include <memory>
#include <string>
#include <vector>

#include "../utils.hpp"
#include "tokens.hpp"

class Lexer {
    std::string m_text;
    size_t m_currentPosition;
    char m_currentChar = '\0';

    explicit Lexer(const std::string& text)
        : m_text(text), m_currentPosition(0) {
        m_currentChar =
            m_text.empty()
                ? '\0'
                : m_text[0];
    };

    void advance();

    Token* makeNumber();
    Result<Token*> makeChar();
    Result<Token*> makeString();
    Token* makeFromLiteral();
    Token* makeOperator();
    Token* makeDelimiter();

    Result<Token*> makeToken();

    template <typename T>
    static Result<T> Ok(T value);

    template <typename T>
    Result<T> Err(const std::string& reason);

    template <typename T>
    static Result<T> Err(Error error);

    std::string getErrorDetails();

  public:
    static Result<std::vector<std::unique_ptr<Token>>> Tokenize(const std::string& text);
};
