#pragma once
#include "AST_nodes.h"

struct Parse_result {
    Node_shared_ptr node;
    Error error;

    Parse_result() : node(nullptr), error() {}
    Parse_result(Node_shared_ptr node_) : node(node_), error() {}
    Parse_result(Error error_) : node(nullptr), error(error_) {}

    bool is_error() { return error.is_error(); }
    void print_error() { error.print_error(); }
};

struct Parser {
    int pos = -1;
    vector<Token_shared_ptr> tokens;
    Token_shared_ptr current_token;
    IN_stream* in;

    Parser() : current_token(nullptr), in(nullptr) {}

    Parser(vector<Token_shared_ptr>& tokens_, IN_stream* in_) : tokens(tokens_), in(in_) { advance(); }

    void advance() {
        pos++;
        current_token = pos < tokens.size() ? tokens[pos] : nullptr;
    }

    Parse_result bin_op(function<Parse_result()> f, vector<char> op) {
        Parse_result res = f();

        if (res.is_error())
            return res;

        Node_shared_ptr left = res.node;
        Token_shared_ptr operation;

        while (current_token != T_END_LINE) {
            bool ok = 0;

            for (char i = 0; i < op.size() && !ok; i++)
                ok = current_token == op[i];

            if (!ok)
                break;

            operation = current_token;
            advance();
            res = f();

            if (res.is_error())
                return res;

            if (res.node == nullptr)
                return Error(
                    ERRORS["Parsing error"],
                    ERRORS["Expression expected"],
                    current_token->pos_start,
                    current_token->pos_end);

            left = make_shared<AST_binary_operation_node>(left, operation, res.node);
        }

        return left;
    }

    Parse_result make_array() {
        Position pos_start = current_token->pos_start;

        if (tokens[pos + 1] == T_BOX_BRACKET_RIGHT) {
            advance();
            return Parse_result(make_shared<AST_array_node>(
                pos_start,
                current_token->pos_end,
                vector<Node_shared_ptr>()));
        }

        vector<Node_shared_ptr> expressions;
        Parse_result res;

        do {
            advance();
            res = expression();

            if (res.is_error())
                return res;

            expressions.push_back(res.node);
        } while (current_token == T_COMMA);

        if (current_token != T_BOX_BRACKET_RIGHT)
            return Error(ERRORS["Parsing error"], ERRORS["Expected"] + " ]", current_token->pos_start, current_token->pos_end);

        return Parse_result(make_shared<AST_array_node>(pos_start, current_token->pos_end, expressions));
    }

    Parse_result make_dict() {
        Position pos_start = current_token->pos_start;

        if (tokens[pos + 1] == T_CURLY_BRACKET_RIGHT) {
            advance();
            return Parse_result(make_shared<AST_dictionary_node>(
                pos_start,
                current_token->pos_end,
                vector<Node_shared_ptr>(),
                vector<Node_shared_ptr>()));
        }

        vector<Node_shared_ptr> keys, values;
        Parse_result res;

        do {
            advance();
            res = expression();

            if (res.is_error())
                return res;

            keys.push_back(res.node);

            if (current_token != T_COLON)
                return Error(ERRORS["Parsing error"], ERRORS["Expected"] + " :", current_token->pos_start, current_token->pos_end);

            advance();
            res = expression();

            if (res.is_error())
                return res;

            values.push_back(res.node);

        } while (current_token == T_COMMA);

        if (current_token != T_CURLY_BRACKET_RIGHT)
            return Error(ERRORS["Parsing error"], ERRORS["Expected"] + " }", current_token->pos_start, current_token->pos_end);

        return Parse_result(make_shared<AST_dictionary_node>(pos_start, current_token->pos_end, keys, values));
    }

    // end_words - the keys for KEYWORDS map
    Parse_result make_body(vector<string> end_words) {
        vector<Node_shared_ptr> body;
        bool cont = true;
        string input;

        do {
            if (current_token != T_END_LINE)
                return Error(
                    ERRORS["Parsing error"],
                    ERRORS["End of line expected"],
                    current_token->pos_start,
                    current_token->pos_end);

            if (in == cin_stream)
                cout << MAP_FILE_NAMES_TO_NO_LINES[PROGRAM_NAME] + 1 << " ... ";

            if (!((*in) >> input))
                return Error(
                    ERRORS["Parsing error"],
                    ERRORS["Reached end of file and not all bodies are closed"],
                    Position(LINES.size() - 1, 0),
                    Position(LINES.size() - 1, LINES.back().text.size()));

            LINES.emplace_back(GLOBAL_CONTEXT.name, input);

            Lexer_result r1 = Lexer(true).make_tokens();

            if (r1.is_error())
                return r1.error;

            Parser p(r1.tokens, in);
            Parse_result r2 = p.expression();

            if (r2.is_error())
                return r2;

            if (p.current_token == T_KEYWORD) {
                string val = p.current_token->value();

                for (auto& word : end_words)
                    if (val == KEYWORDS[word]) {
                        cont   = false;
                        tokens = p.tokens;
                        pos    = p.pos - 1;
                        advance();
                        break;
                    }
            }

            if (cont && p.current_token != T_END_LINE)
                return Error(
                    ERRORS["Parsing error"],
                    ERRORS["Incorrect instruction"],
                    current_token->pos_start,
                    current_token->pos_end);

            if (r2.node != nullptr)
                body.push_back(r2.node);
        } while (cont);

        return Parse_result(make_shared<AST_body_node>(body));
    }

    Parse_result make_if() {
        advance();

        Parse_result condition = expression();

        if (condition.is_error())
            return condition;

        if (condition.node == nullptr)
            return Error(
                ERRORS["Parsing error"],
                ERRORS["Expression expected"],
                current_token->pos_start,
                current_token->pos_end);

        if (current_token != T_KEYWORD || current_token->value() != KEYWORDS["then"])
            return Error(
                ERRORS["Parsing error"],
                ERRORS["Expected"] + ' ' + KEYWORDS["then"],
                current_token->pos_start,
                current_token->pos_end);

        advance();

        Parse_result body = make_body({ "else", "end" }), else_body;

        if (body.is_error())
            return body;

        if (body.node == nullptr)
            return Error(ERRORS["Parsing error"], ERRORS["Expression expected"], current_token->pos_start, current_token->pos_end);

        if (current_token == T_KEYWORD && current_token->value() == KEYWORDS["else"]) {
            advance();
            else_body = make_body({ "end" });

            if (else_body.is_error())
                return else_body;

            if (else_body.node == nullptr)
                return Error(ERRORS["Parsing error"], ERRORS["Expression expected"], current_token->pos_start, current_token->pos_end);
        }

        if (current_token != T_KEYWORD || current_token->value() != KEYWORDS["end"])
            return Error(
                ERRORS["Parsing error"],
                ERRORS["Expected"] + ' ' + KEYWORDS["end"],
                current_token->pos_start,
                current_token->pos_end);

        advance();

        return Parse_result(make_shared<AST_if_node>(condition.node, body.node, else_body.node));
    }

    Parse_result make_for() {
        advance();

        if (current_token != T_IDENTIFIER)
            return Error(
                ERRORS["Parsing error"],
                ERRORS["Identifier expected"],
                current_token->pos_start,
                current_token->pos_end);

        Token_shared_ptr variable = current_token;

        Parse_result assignment =
            tokens[pos + 1] == T_BOX_BRACKET_LEFT
                ? make_assignment_index(false)
                : make_assignment_variable(false);

        if (assignment.is_error())
            return assignment;

        if (current_token != T_COMMA)
            return Error(ERRORS["Parsing error"], ERRORS["Expected"] + " ,", current_token->pos_start, current_token->pos_end);

        advance();

        Parse_result stop = expression(), skip;

        if (stop.is_error())
            return stop;

        if (stop.node == nullptr)
            return Error(ERRORS["Parsing error"], ERRORS["Expression expected"], current_token->pos_start, current_token->pos_end);

        if (current_token == T_COMMA) {
            advance();

            skip = expression();

            if (skip.is_error())
                return skip;

            if (skip.node == nullptr)
                return Error(ERRORS["Parsing error"], ERRORS["Expression expected"], current_token->pos_start, current_token->pos_end);
        }

        if (current_token != T_KEYWORD || current_token->value() != KEYWORDS["execute"])
            return Error(
                ERRORS["Parsing error"],
                ERRORS["Expected"] + ' ' + KEYWORDS["execute"],
                current_token->pos_start,
                current_token->pos_end);

        advance();

        Parse_result body = make_body({ "end" });

        if (body.is_error())
            return body;

        if (body.node == nullptr)
            return Error(ERRORS["Parsing error"], ERRORS["Expression expected"], current_token->pos_start, current_token->pos_end);

        if (current_token != T_KEYWORD || current_token->value() != KEYWORDS["end"])
            return Error(
                ERRORS["Parsing error"],
                ERRORS["Expected"] + ' ' + KEYWORDS["end"],
                current_token->pos_start,
                current_token->pos_end);

        advance();

        return Parse_result(make_shared<AST_for_node>(assignment.node, stop.node, skip.node, body.node));
    }

    Parse_result
    make_while() {
        advance();

        Parse_result condition = expression();

        if (condition.is_error())
            return condition;

        if (condition.node == nullptr)
            return Error(ERRORS["Parsing error"], ERRORS["Expression expected"], current_token->pos_start, current_token->pos_end);

        if (current_token != T_KEYWORD || current_token->value() != KEYWORDS["execute"])
            return Error(
                ERRORS["Parsing error"],
                ERRORS["Expected"] + ' ' + KEYWORDS["execute"],
                current_token->pos_start,
                current_token->pos_end);

        advance();

        Parse_result body = make_body({ "end" });

        if (body.is_error())
            return body;

        if (body.node == nullptr)
            return Error(ERRORS["Parsing error"], ERRORS["Expression expected"], current_token->pos_start, current_token->pos_end);

        if (current_token != T_KEYWORD || current_token->value() != KEYWORDS["end"])
            return Error(
                ERRORS["Parsing error"],
                ERRORS["Expected"] + ' ' + KEYWORDS["end"],
                current_token->pos_start,
                current_token->pos_end);

        advance();

        return Parse_result(make_shared<AST_while_node>(condition.node, body.node));
    }

    Parse_result make_do_until() {
        advance();

        Parse_result body = make_body({ "until" });

        if (body.is_error())
            return body;

        if (body.node == nullptr)
            return Error(ERRORS["Parsing error"], ERRORS["Expression expected"], current_token->pos_start, current_token->pos_end);

        if (current_token != T_KEYWORD || current_token->value() != KEYWORDS["until"])
            return Error(
                ERRORS["Parsing error"],
                ERRORS["Expected"] + ' ' + KEYWORDS["until"],
                current_token->pos_start,
                current_token->pos_end);

        advance();

        Parse_result condition = expression();

        if (condition.is_error())
            return condition;

        if (condition.node == nullptr)
            return Error(ERRORS["Parsing error"], ERRORS["Expression expected"], current_token->pos_start, current_token->pos_end);

        return Parse_result(make_shared<AST_do_until_node>(condition.node, body.node));
    }

    Parse_result make_try() {
        advance();

        Parse_result try_body = make_body({ "catch", "end" }), catch_body;

        if (try_body.is_error())
            return try_body;

        if (try_body.node == nullptr)
            return Error(ERRORS["Parsing error"], ERRORS["Expression expected"], current_token->pos_start, current_token->pos_end);

        if (current_token == T_KEYWORD && current_token->value() == KEYWORDS["catch"]) {
            advance();
            catch_body = make_body({ "end" });

            if (catch_body.is_error())
                return catch_body;

            if (catch_body.node == nullptr)
                return Error(ERRORS["Parsing error"], ERRORS["Expression expected"], current_token->pos_start, current_token->pos_end);
        }

        if (current_token != T_KEYWORD || current_token->value() != KEYWORDS["end"])
            return Error(
                ERRORS["Parsing error"],
                ERRORS["Expected"] + ' ' + KEYWORDS["end"],
                current_token->pos_start,
                current_token->pos_end);

        advance();

        return Parse_result(make_shared<AST_try_node>(try_body.node, catch_body.node));
    }

    Parse_result make_funtion() {
        advance();

        Token_shared_ptr identifier;

        if (current_token == T_IDENTIFIER)
            identifier = current_token;

        advance();

        if (current_token != T_ROUND_BRACKET_LEFT)
            return Error(ERRORS["Parsing error"], ERRORS["Expected"] + " (", current_token->pos_start, current_token->pos_end);

        advance();

        vector<Token_shared_ptr> args;

        if (current_token == T_IDENTIFIER) {
            args.push_back(current_token);

            advance();

            while (current_token == T_COMMA) {
                advance();

                if (current_token != T_IDENTIFIER)
                    return Error(
                        ERRORS["Parsing error"],
                        ERRORS["Identifier expected"],
                        current_token->pos_start,
                        current_token->pos_end);

                args.push_back(current_token);
                advance();
            }
        }

        if (current_token != T_ROUND_BRACKET_RIGHT)
            return Error(ERRORS["Parsing error"], ERRORS["Expected"] + " )", current_token->pos_start, current_token->pos_end);

        advance();

        if (current_token != T_COLON)
            return Error(ERRORS["Parsing error"], ERRORS["Expected"] + " :", current_token->pos_start, current_token->pos_end);

        advance();

        Parse_result body = make_body({ "end" });

        if (body.is_error())
            return body;

        if (body.node == nullptr)
            return Error(ERRORS["Parsing error"], ERRORS["Expression expected"], current_token->pos_start, current_token->pos_end);

        if (current_token != T_KEYWORD || current_token->value() != KEYWORDS["end"])
            return Error(
                ERRORS["Parsing error"],
                ERRORS["Expected"] + ' ' + KEYWORDS["end"],
                current_token->pos_start,
                current_token->pos_end);

        advance();

        return Parse_result(make_shared<AST_function_definition_node>(identifier, body.node, args));
    }

    Parse_result make_index_access(Parse_result res) {
        advance();

        Node_shared_ptr arr = res.node;
        res                 = expression();

        if (res.is_error())
            return res;

        if (current_token != T_BOX_BRACKET_RIGHT)
            return Error(ERRORS["Parsing error"], ERRORS["Expected"] + " ]", current_token->pos_start, current_token->pos_end);

        advance();

        return Parse_result(make_shared<AST_index_access_node>(arr, res.node));
    }

    Parse_result make_function_call(Parse_result res) {
        advance();

        vector<Node_shared_ptr> args;

        Parse_result arg = expression();

        if (arg.node != nullptr) {
            args.push_back(arg.node);

            while (current_token == T_COMMA) {
                advance();

                arg = expression();

                if (arg.is_error())
                    return arg;

                if (arg.node == nullptr)
                    return Error(
                        ERRORS["Parsing error"],
                        ERRORS["Expression expected"],
                        current_token->pos_start,
                        current_token->pos_end);

                args.push_back(arg.node);
            }
        }

        if (current_token != T_ROUND_BRACKET_RIGHT)
            return Error(ERRORS["Parsing error"], ERRORS["Expected"] + " )", current_token->pos_start, current_token->pos_end);

        advance();
        return Parse_result(make_shared<AST_function_call_node>(res.node, args));
    }

    Parse_result make_print() {
        vector<Node_shared_ptr> nodes;
        Parse_result res;

        do {
            advance();
            res = expression();

            if (res.is_error())
                return res;

            if (res.node == nullptr)
                return Error(ERRORS["Parsing error"], ERRORS["Expression expected"], current_token->pos_start, current_token->pos_end);

            nodes.push_back(res.node);
        } while (current_token == T_COMMA);

        if (current_token != T_COLON)
            return Parse_result(make_shared<AST_write_node>(nodes));

        advance();

        if (current_token != T_IDENTIFIER)
            return Error(ERRORS["Parsing error"], ERRORS["Identifier expected"], current_token->pos_start, current_token->pos_end);

        Token_shared_ptr identif = current_token;

        advance();

        return Parse_result(make_shared<AST_write_node>(nodes, identif));
    }

    Parse_result make_read() {
        vector<pair<Token_shared_ptr, vector<Node_shared_ptr>>> reads;
        vector<Node_shared_ptr> indeces;
        // identifier and indeces

        do {
            advance();

            if (current_token != T_IDENTIFIER)
                return Error(ERRORS["Parsing error"], ERRORS["Identifier expected"], current_token->pos_start, current_token->pos_end);

            Token_shared_ptr variable = current_token;

            advance();

            indeces.clear();

            while (current_token == T_BOX_BRACKET_LEFT) {
                advance();
                Position pos     = current_token->pos_start;
                Parse_result idx = expression();

                if (idx.is_error())
                    return idx;

                if (idx.node == nullptr)
                    return Error(ERRORS["Parsing error"], ERRORS["Expression expected"], current_token->pos_start, current_token->pos_end);

                if (current_token != T_BOX_BRACKET_RIGHT)
                    return Error(ERRORS["Parsing error"], ERRORS["Expected"] + " ]", current_token->pos_start, current_token->pos_end);

                advance();
                indeces.push_back(idx.node);
            }

            reads.emplace_back(variable, indeces);
        } while (current_token == T_COMMA);

        if (current_token != T_COLON)
            return Parse_result(make_shared<AST_read_node>(reads));

        advance();

        if (current_token != T_IDENTIFIER)
            return Error(ERRORS["Parsing error"], ERRORS["Identifier expected"], current_token->pos_start, current_token->pos_end);

        Token_shared_ptr file = current_token;

        advance();

        return Parse_result(make_shared<AST_read_node>(reads, file));
    }

    Parse_result base() {
        Parse_result res;

        // schimbat cu switch
        if (current_token == T_NUMBER) {
            res.node = make_shared<AST_number_node>(current_token);
            advance();
        } else if (current_token == T_CHAR) {
            res.node = make_shared<AST_char_node>(current_token);
            advance();
        } else if (current_token == T_STRING) {
            res.node = make_shared<AST_string_node>(current_token);
            advance();
        } else if (current_token == T_IDENTIFIER) {
            res.node = make_shared<AST_variable_access_node>(current_token);
            advance();
        } else if (current_token == T_MINUS || current_token == T_PLUS) {
            Token_shared_ptr t = current_token;
            advance();
            res = base();

            if (res.is_error())
                return res;

            if (res.node == nullptr)
                return Error(ERRORS["Parsing error"], ERRORS["Expression expected"], t->pos_start, t->pos_end);

            res.node = make_shared<AST_unary_operation_node>(res.node, t);
        } else if (current_token == T_ROUND_BRACKET_LEFT) {
            advance();
            res = expression();

            if (res.is_error())
                return res.error;

            if (current_token != T_ROUND_BRACKET_RIGHT)
                return Error(ERRORS["Parsing error"], ERRORS["Expected"] + " )", current_token->pos_start, current_token->pos_end);

            advance();
        } else if (current_token == T_BOX_BRACKET_LEFT) {
            res = make_array();

            if (res.is_error())
                return res;

            advance();
        } else if (current_token == T_CURLY_BRACKET_LEFT) {
            res = make_dict();

            if (res.is_error())
                return res;

            advance();
        } else if (current_token == T_KEYWORD) {
            string keyword = current_token->value();

            if (keyword == KEYWORDS["if"])
                return make_if();
            else if (keyword == KEYWORDS["for"])
                return make_for();
            else if (keyword == KEYWORDS["while"])
                return make_while();
            else if (keyword == KEYWORDS["do"])
                return make_do_until();
            else if (keyword == KEYWORDS["print"])
                return make_print();
            else if (keyword == KEYWORDS["read"])
                return make_read();
            else if (keyword == KEYWORDS["try"])
                return make_try();
            else if (keyword == KEYWORDS["function"])
                return make_funtion();
            else if (keyword == KEYWORDS["return"]) {
                advance();
                Parse_result node = expression();

                if (node.is_error())
                    return node;

                return Parse_result(make_shared<AST_return_node>(node.node));
            } else if (keyword == KEYWORDS["break"]) {
                Position p1 = current_token->pos_start, p2 = current_token->pos_end;
                advance();
                return Parse_result(make_shared<AST_break_continue_node>(true, p1, p2));
            } else if (keyword == KEYWORDS["continue"]) {
                Position p1 = current_token->pos_start, p2 = current_token->pos_end;
                advance();
                return Parse_result(make_shared<AST_break_continue_node>(false, p1, p2));
            } else if (keyword == KEYWORDS["include"]) {
                advance();
                if (current_token != T_STRING)
                    return Error(
                        ERRORS["Parsing error"],
                        ERRORS["Expected"] + ' ' + *VALUE_NAMES_ARRAY[V_STRING],
                        current_token->pos_start,
                        current_token->pos_end);

                Token_shared_ptr t = current_token;
                advance();
                return Parse_result(make_shared<AST_include_node>(t));
            } else if (keyword == KEYWORDS["run"]) {
                advance();
                if (current_token != T_STRING)
                    return Error(
                        ERRORS["Parsing error"],
                        ERRORS["Expected"] + ' ' + *VALUE_NAMES_ARRAY[V_STRING],
                        current_token->pos_start,
                        current_token->pos_end);

                Token_shared_ptr t = current_token;
                advance();
                return Parse_result(make_shared<AST_run_node>(t));
            } else
                return Parse_result();

            if (res.is_error())
                return res;

        } else
            return Error(ERRORS["Parsing error"], ERRORS["Expression expected"], current_token->pos_start, current_token->pos_end);

        while ((current_token == T_BOX_BRACKET_LEFT || current_token == T_ROUND_BRACKET_LEFT) && !res.is_error())
            res = current_token == T_BOX_BRACKET_LEFT ? make_index_access(res) : make_function_call(res);

        return res;
    }

    Parse_result factor() { return bin_op(bind(&Parser::base, this), { T_MULTIPLY, T_DIVIDE, T_MODULO }); }

    Parse_result term() { return bin_op(bind(&Parser::factor, this), { T_PLUS, T_MINUS }); }

    Parse_result comparison() {
        return bin_op(
            bind(&Parser::term, this),
            { T_EQUALS, T_LESS, T_LESS_EQUAL, T_GREATER, T_GREATER_EQUAL, T_DIFFERENT });
    }

    Parse_result make_assignment_variable(bool is_const) {
        Token_shared_ptr variable_token = current_token;
        advance();

        if (current_token != T_ASSIGNMENT)
            return Error(ERRORS["Parsing error"], ERRORS["Assignment expected"], current_token->pos_start, current_token->pos_end);

        advance();

        Parse_result res = expression();

        if (res.is_error())
            return res;

        if (res.node == nullptr)
            return Error(ERRORS["Parsing error"], ERRORS["Expression expected"], current_token->pos_start, current_token->pos_end);

        return Parse_result(make_shared<AST_variable_assign_node>(variable_token, res.node, is_const));
    }

    Parse_result make_assignment_index(bool is_const) {
        Parse_result res;
        Node_shared_ptr var         = make_shared<AST_variable_access_node>(current_token);
        Token_shared_ptr identifier = current_token;
        advance();

        while (current_token == T_BOX_BRACKET_LEFT) {
            advance();

            res = expression();

            if (res.is_error())
                return res;

            if (current_token != T_BOX_BRACKET_RIGHT)
                return Error(ERRORS["Parsing error"], ERRORS["Expected"] + " ]", current_token->pos_start, current_token->pos_end);

            var = make_shared<AST_index_access_node>(var, res.node);
            advance();
        }

        if (current_token != T_ASSIGNMENT)
            return Error(ERRORS["Parsing error"], ERRORS["Assignment expected"], current_token->pos_start, current_token->pos_end);

        advance();

        Parse_result val = expression();

        if (val.is_error())
            return val;

        if (val.node == nullptr)
            return Error(ERRORS["Parsing error"], ERRORS["Expression expected"], current_token->pos_start, current_token->pos_end);

        // need to assign at last index not access it
        AST_index_access_node* a = static_cast<AST_index_access_node*>(var.get());
        res.node                 = make_shared<AST_index_assign_node>(identifier, a->arr, a->index, val.node, is_const);

        return res;
    }

    Parse_result expression() {
        if (current_token == T_END_LINE)
            return Parse_result();

        bool is_const = false;

        if (current_token == T_KEYWORD)
            if (current_token->value() == KEYWORDS["const"]) {
                is_const = true;
                advance();
            }

        if (current_token == T_IDENTIFIER) {
            int p = pos - 1;

            Parse_result res =
                tokens[pos + 1] == T_BOX_BRACKET_LEFT
                    ? make_assignment_index(is_const)
                    : make_assignment_variable(is_const);

            if (!res.is_error())
                return res;

            if (res.error.details != ERRORS["Assignment expected"])
                return res;

            if (is_const)
                return Parse_result();

            pos = p;
            advance();
        }

        return bin_op(bind(&Parser::comparison, this), { T_AND, T_OR });
    }

    Parse_result parse() {
        Parse_result rez = expression();

        if (rez.is_error())
            return rez;

        if (current_token != T_END_LINE)
            return Error(ERRORS["Parsing error"], ERRORS["Incorrect instruction"], current_token->pos_start, current_token->pos_end);

        return rez;
    }
};
