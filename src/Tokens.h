#pragma once
#include "Base.h"

struct Token {
    char type;
    Position pos_start, pos_end;

    Token(char type_ = T_NULL) : type(type_) {}

    void set_position(Position pos_start_, Position pos_end_) {
        pos_start = pos_start_;
        pos_end   = pos_end_;
    }

    virtual string value() { return "NULL"; }
};

typedef shared_ptr<Token> Token_shared_ptr;

bool operator==(const Token_shared_ptr& t, const char type) {
    return t->type == type;
}

bool operator!=(const Token_shared_ptr& t, const char type) {
    return t->type != type;
}

struct Number_token : public Token {
    double value_;

    Number_token(double value_) : value_(value_) {
        type = T_NUMBER;
    }

    string value() override { return double_to_string(value_); }
};

struct Char_token : public Token {
    char value_;

    Char_token(char char_) : value_(char_) {
        type = T_CHAR;
    }

    string value() override { return string(1, value_); }
};

struct String_token : public Token {
    string value_;

    String_token(string value_, char type_) : value_(value_) {
        type = type_;
    }

    string value() override { return value_; }
};
