#pragma once

#include <ostream>
#include <string>

struct Token {
    enum class Kind {
        // literals
        Number,
        Char,
        String,
        Identifier,

        // constants
        Null,
        True,
        False,

        // keywords
        Let,
        If,
        Then,
        Else,
        End,
        For,
        Execute,
        While,
        Do,
        Until,
        Print,
        Read,
        Throw,
        Try,
        Catch,
        Function,
        Return,
        Continue,
        Break,
        Include,
        Run,

        // operators
        Plus,
        Minus,
        Multiply,
        Divide,
        Modulo,
        Equals,
        Less,
        LessEqual,
        Greater,
        GreaterEqual,
        Different,
        Assignment,
        Or,
        And,

        // delimiters
        ParenLeft,
        ParenRight,
        BracketLeft,
        BracketRight,
        CurlyLeft,
        CurlyRight,
        Comma,
        Colon,
        Endline,
        EOI,  // end of input
    };

    const Kind kind;
    const std::string value;

    Token(const Kind& tokenKind, const std::string& tokenValue)
        : kind(tokenKind), value(tokenValue) {}
};

std::ostream& operator<<(std::ostream& os, Token::Kind kind);
