#pragma once
#include "Tokens.h"

struct Lexer_result {
    vector<Token_shared_ptr> tokens;
    Error error;

    Lexer_result(Error error_ = Error()) : error(error_) {}
    Lexer_result(vector<Token_shared_ptr> tokens_) : tokens(tokens_) {}
    Lexer_result(Token_shared_ptr token) {
        tokens.push_back(token);
    }

    bool is_error() { return error.is_error(); }
    void print_error() { error.print_error(); }
};

struct Lexer {
    string text;
    Position current_position;
    char current_char = '\0';

    void advance() {
        current_position.advance();
        current_char = current_position.column < text.size() ? text[current_position.column] : '\0';
    }

    Lexer(bool active) {
        if (!active)
            return;

        text             = LINES.back().text;
        current_position = Position(LINES.size() - 1);
        advance();
    }

    Token_shared_ptr make_number() {
        string number;

        while (current_char >= '0' && current_char <= '9') {
            number += current_char;
            advance();
        }

        if (current_char != '.')
            return make_shared<Number_token>(stod(number));

        if (number.size() == 0)
            number += '0';

        number += '.';
        advance();

        while (current_char >= '0' && current_char <= '9') {
            number += current_char;
            advance();
        }

        return make_shared<Number_token>(stod(number));
    }

    Lexer_result make_char() {
        Position pos = current_position;
        advance();
        char ch;

        if (current_char == '\\') {
            advance();
            ch = ESCAPED_CHARS[current_char];
        } else
            ch = current_char;

        advance();

        if (current_char != '\'') {
            string details = current_char == '\0' ? ERRORS["Expected"] + " '" : ERRORS["Use \"\" for string"];
            return Error(ERRORS["Lexing error"], details, pos, current_position);
        }

        advance();
        Token_shared_ptr t = make_shared<Char_token>(ch);
        t->set_position(pos, current_position);
        return t;
    }

    Lexer_result make_string() {
        Position pos = current_position;
        advance();
        string str;
        bool is_escaped = false;

        while ((current_char != '"' || is_escaped) && current_char) {
            if (is_escaped) {
                if (ESCAPED_CHARS.find(current_char) != ESCAPED_CHARS.end())
                    str += ESCAPED_CHARS[current_char];
                else
                    str += current_char;

                is_escaped = false;
            } else if (current_char == '\\')
                is_escaped = true;
            else
                str += current_char;
            advance();
        }

        if (current_char != '"')
            return Error(ERRORS["Lexing error"],
                         ERRORS["Expected"] + " \"", pos, current_position);

        advance();
        Token_shared_ptr t = make_shared<String_token>(str, T_STRING);
        t->set_position(pos, current_position);
        return t;
    }

    Token_shared_ptr make_identifier() {
        string identif;

        while (strchr(IDENTIFIER_CHARS, current_char) && current_char) {
            identif += current_char;
            advance();
        }

        bool cont = false;

        if (current_char == ' ')
            for (auto& i : KEYWORDS)
                if (i.second.find(" ") != string::npos) {
                    string check;

                    for (auto& j : i.second) {
                        if (j == ' ')
                            break;
                        check += j;
                    }

                    if (check == identif) {
                        identif += ' ';
                        cont = true;
                        advance();
                    }
                }

        if (cont)
            while (strchr(IDENTIFIER_CHARS, current_char) && current_char) {
                identif += current_char;
                advance();
            }

        if (identif == KEYWORDS["and"])
            return make_shared<Token>(T_AND);

        if (identif == KEYWORDS["or"])
            return make_shared<Token>(T_OR);

        for (auto& i : KEYWORDS)
            if (i.second == identif)
                return make_shared<String_token>(identif, T_KEYWORD);

        return make_shared<String_token>(identif, T_IDENTIFIER);
    }

    Lexer_result make_tokens() {
        vector<Token_shared_ptr> tokens;
        Token_shared_ptr t;
        Position pos;

        while (current_char) {
            if (current_char == COMMENT_CHAR)
                break;

            if (strchr(" \t\r", current_char)) {
                advance();
                continue;
            }

            pos = current_position;

            if (OPERATIONS.find(current_char) != OPERATIONS.end()) {
                char type = OPERATIONS[current_char];
                advance();
                t = make_shared<Token>(type);
                t->set_position(pos, current_position);
                tokens.push_back(t);
                continue;
            }

            // number
            if (current_char >= '0' && current_char <= '9') {
                t = make_number();
                t->set_position(pos, current_position);
                tokens.push_back(t);
                continue;
            }

            // char
            if (current_char == '\'') {
                Lexer_result r = make_char();

                if (r.is_error())
                    return r.error;

                tokens.push_back(r.tokens[0]);
                continue;
            }

            // string
            if (current_char == '"') {
                Lexer_result r = make_string();

                if (r.is_error())
                    return r.error;

                tokens.push_back(r.tokens[0]);
                continue;
            }

            // identifier or keyword
            if (strchr(IDENTIFIER_CHARS, current_char) &&
                (current_char < '0' || current_char > '9')) {
                t = make_identifier();
                t->set_position(pos, current_position);
                tokens.push_back(t);
                continue;
            }

            // less < , less or equal <= , assignment <-, different <> operators
            if (current_char == '<') {
                advance();

                switch (current_char) {
                    case '=': t = make_shared<Token>(T_LESS_EQUAL); break;
                    case '-': t = make_shared<Token>(T_ASSIGNMENT); break;
                    case '>': t = make_shared<Token>(T_DIFFERENT); break;
                    default: {
                        current_position.column--;
                        t = make_shared<Token>(T_LESS);
                    } break;
                }
                advance();

                t->set_position(pos, current_position);
                tokens.push_back(t);

                continue;
            }

            // greater > , greater or equals >= operators
            if (current_char == '>') {
                advance();
                if (current_char == '=') {
                    advance();
                    t = make_shared<Token>(T_GREATER_EQUAL);
                } else
                    t = make_shared<Token>(T_GREATER);

                t->set_position(pos, current_position);
                tokens.push_back(t);

                continue;
            }

            pos.advance();
            return Error(ERRORS["Lexing error"], ERRORS["Invalid syntax"], current_position, pos);
        }

        t            = make_shared<Token>(T_END_LINE);
        t->pos_start = current_position;
        advance();
        t->pos_end = current_position;
        tokens.push_back(t);
        return tokens;
    }
};
