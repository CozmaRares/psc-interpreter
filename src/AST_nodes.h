#pragma once
#include <queue>

#include "Lexer.h"
#include "Values.h"

struct AST_number_node : public AST_node {
    double value_;

    AST_number_node(Token_shared_ptr token_)
        : value_(stod(token_->value())) {
        pos_start = token_->pos_start;
        pos_end   = token_->pos_end;
    }

    Interpreter_result visit(Context* context) override {
        Value_shared_ptr nr = make_shared<Number_value>(value_);
        nr->set_position(pos_start, pos_end);
        nr->context = context;
        return nr;
    }
};
struct AST_char_node : public AST_node {
    char value_;

    AST_char_node(Token_shared_ptr token_)
        : value_(token_->value()[0]) {
        pos_start = token_->pos_start;
        pos_end   = token_->pos_end;
    }

    Interpreter_result visit(Context* context) override {
        Value_shared_ptr chr = make_shared<Char_value>(value_);
        chr->set_position(pos_start, pos_end);
        chr->context = context;
        return chr;
    }
};

struct AST_array_node : public AST_node {
    vector<Node_shared_ptr> arr;

    AST_array_node(Position pos_start_, Position pos_end_, vector<Node_shared_ptr> arr_)
        : arr(arr_) {
        pos_start = pos_start_;
        pos_end   = pos_end_;
    }

    Interpreter_result visit(Context* context) override {
        vector<Value_shared_ptr> value_arr;
        Interpreter_result res;

        for (auto& elem : arr) {
            res = elem->visit(context);

            if (res.is_error())
                return res;

            value_arr.push_back(res.value);
        }

        Value_shared_ptr arr_ = make_shared<Array_value>(value_arr);
        arr_->set_position(pos_start, pos_end);
        arr_->context = context;
        return arr_;
    }
};

struct AST_string_node : public AST_node {
    string value_;

    AST_string_node(Token_shared_ptr token_)
        : value_(token_->value()) {
        pos_start = token_->pos_start;
        pos_end   = token_->pos_end;
    }

    Interpreter_result visit(Context* context) override {
        Value_shared_ptr str = make_shared<String_value>(value_);
        str->set_position(pos_start, pos_end);
        str->context = context;
        return str;
    }
};

struct AST_dictionary_node : public AST_node {
    vector<Node_shared_ptr> keys, values;

    AST_dictionary_node(
        Position pos_start_,
        Position pos_end_,
        vector<Node_shared_ptr> keys_,
        vector<Node_shared_ptr> values_)
        : keys(keys_), values(values_) {
        pos_start = pos_start_;
        pos_end   = pos_end_;
    }

    Interpreter_result visit(Context* context) override {
        map<string, Value_shared_ptr> m;
        Interpreter_result key, value;

        for (int i = 0; i < keys.size(); i++) {
            key = keys[i]->visit(context);

            if (key.is_error())
                return key;

            switch (key.value->type) {
                case V_NUMBER: break;
                case V_CHAR: break;
                case V_STRING: break;
                default: {
                    ostringstream details;
                    details << ERRORS["Expected"] << ' ' << VALUE_NAMES_MAP["NUMBER"]
                            << ", " << VALUE_NAMES_MAP["CHAR"] << ", " << VALUE_NAMES_MAP["STRING"];

                    return RT_Error(context, details.str(), keys[i]->pos_start, keys[i]->pos_end);
                }
            }

            value = values[i]->visit(context);

            if (value.is_error())
                return value;

            m[key.value->repr()] = value.value;
        }

        Value_shared_ptr dict = make_shared<Dictionary_value>(m);
        dict->set_position(pos_start, pos_end);
        dict->context = context;
        return dict;
    }
};

struct AST_binary_operation_node : public AST_node {
    Node_shared_ptr left, right;
    char operation;

    AST_binary_operation_node(Node_shared_ptr left_, Token_shared_ptr operation_, Node_shared_ptr right_)
        : left(left_), operation(operation_->type), right(right_) {
        pos_start = left_->pos_start;
        pos_end   = right_->pos_end;
    }

    Interpreter_result visit(Context* context) override {
        Interpreter_result left_part  = left->visit(context);
        Interpreter_result right_part = right->visit(context);

        if (left_part.is_error())
            return left_part;

        if (right_part.is_error())
            return right_part;

        Value_shared_ptr st = left_part.value, dr = right_part.value;

        st->context = context;
        st->set_position(left->pos_start, left->pos_end);
        dr->context = context;
        dr->set_position(right->pos_start, right->pos_end);

        Interpreter_result error;

        switch (operation) {
            case T_OR: error = st->or_(dr); break;
            case T_AND: error = st->and_(dr); break;
            case T_LESS: error = st->less(dr); break;
            case T_PLUS: error = st->add(dr); break;
            case T_MINUS: error = st->subtract(dr); break;
            case T_DIVIDE: error = st->divide(dr); break;
            case T_EQUALS: error = st->equals(dr); break;
            case T_MODULO: error = st->modulo(dr); break;
            case T_GREATER: error = st->greater(dr); break;
            case T_MULTIPLY: error = st->multiply(dr); break;
            case T_DIFFERENT: error = st->different(dr); break;
            case T_LESS_EQUAL: error = st->less_or_equal(dr); break;
            case T_GREATER_EQUAL: error = st->greater_or_equal(dr); break;
            default: break;
        }

        error.print = true;

        return error;
    }
};

struct AST_unary_operation_node : public AST_node {
    Node_shared_ptr node;
    char operation;

    AST_unary_operation_node(Node_shared_ptr node_, Token_shared_ptr operation_)
        : node(node_), operation(operation_->type) {
        pos_start = operation_->pos_start;
        pos_end   = node_->pos_end;
    }

    Interpreter_result visit(Context* context) override {
        Interpreter_result res = node->visit(context);

        if (res.is_error())
            return res;

        Value_shared_ptr n = res.value;
        n->set_position(node->pos_start, node->pos_end);
        n->context = context;

        Interpreter_result error;

        switch (operation) {
            case T_PLUS: error = n; break;
            case T_MINUS: error = n->multiply(make_shared<Number_value>(-1.0)); break;
            default: break;
        }

        error.print = true;

        return error;
    }
};

struct AST_variable_assign_node : public AST_node {
    Token_shared_ptr identifier;
    Node_shared_ptr variable_value;
    bool is_const;

    AST_variable_assign_node(Token_shared_ptr identifier_, Node_shared_ptr variable_value_, bool is_const_)
        : identifier(identifier_), variable_value(variable_value_), is_const(is_const_) {}

    Interpreter_result visit(Context* context) override {
        Interpreter_result res = variable_value->visit(context);

        if (res.is_error())
            return res;

        string variable_name = identifier->value();

        for (auto& i : context->in_files)
            if (variable_name == i.first)
                return RT_Error(context, ERRORS["File already opened"], identifier->pos_start, identifier->pos_end);

        for (auto& i : context->out_files)
            if (variable_name == i.first)
                return RT_Error(context, ERRORS["File already opened"], identifier->pos_start, identifier->pos_end);

        if (context->memory.is(variable_name))
            if (context->memory.get(variable_name)->is_const)
                return RT_Error(context, ERRORS["Constant variable"], identifier->pos_start, identifier->pos_end);

        res.value->is_const = is_const;
        context->memory.set(variable_name, res.value->copy());
        res.value = context->memory.get(variable_name);
        res.print = false;
        return res;
    }
};

struct AST_variable_access_node : public AST_node {
    string variable_name;

    AST_variable_access_node(Token_shared_ptr variable)
        : variable_name(variable->value()) {
        pos_start = variable->pos_start;
        pos_end   = variable->pos_end;
    }

    Interpreter_result visit(Context* context) override {
        for (auto& i : context->in_files)
            if (variable_name == i.first)
                return RT_Error(context, ERRORS["Cannot reference files"], pos_start, pos_end);

        for (auto& i : context->out_files)
            if (variable_name == i.first)
                return RT_Error(context, ERRORS["Cannot reference files"], pos_start, pos_end);

        if (!context->memory.is(variable_name, &GLOBAL_CONTEXT.memory))
            return RT_Error(context, ERRORS["Unknown identifier"], pos_start, pos_end);

        return context->memory.get(variable_name, &GLOBAL_CONTEXT.memory);
    }
};

struct AST_index_access_node : public AST_node {
    Node_shared_ptr arr, index;

    AST_index_access_node(Node_shared_ptr arr_, Node_shared_ptr index_)
        : arr(arr_), index(index_) {
        pos_start = arr->pos_start;
        pos_end   = index->pos_end;
    }

    Interpreter_result visit(Context* context) override {
        Interpreter_result res1 = arr->visit(context);

        if (res1.is_error())
            return res1;

        Interpreter_result res2 = index->visit(context);

        if (res2.is_error())
            return res2;

        res2.value          = res2.value->copy();
        res2.value->context = context;
        res2.value->set_position(index->pos_start, index->pos_end);

        return res1.value->get_at_index(res2.value);
    }
};

struct AST_index_assign_node : public AST_node {
    Token_shared_ptr identifier;
    Node_shared_ptr arr, index, value;
    bool is_const;

    AST_index_assign_node(
        Token_shared_ptr identifier_,
        Node_shared_ptr arr_,
        Node_shared_ptr index_,
        Node_shared_ptr value_,
        bool is_const_)
        : identifier(identifier_), arr(arr_), index(index_), value(value_), is_const(is_const_) {}

    Interpreter_result visit(Context* context) override {
        {
            string variable_name = identifier->value();
            if (context->memory.is(variable_name))
                if (context->memory.get(variable_name)->is_const)
                    return RT_Error(context, ERRORS["Constant variable"], identifier->pos_start, identifier->pos_end);
        }

        Interpreter_result res1 = arr->visit(context);

        if (res1.is_error())
            return res1;

        Interpreter_result res2 = index->visit(context);

        if (res2.is_error())
            return res2;

        res2.value          = res2.value->copy();
        res2.value->context = context;
        res2.value->set_position(index->pos_start, index->pos_end);

        Interpreter_result res3 = value->visit(context);

        if (res3.is_error())
            return res3;

        return res1.value->set_at_index(res2.value, res3.value->copy(), is_const);
    }
};

struct AST_if_node : public AST_node {
    Node_shared_ptr condition, body, else_body;

    AST_if_node(Node_shared_ptr condition_, Node_shared_ptr body_, Node_shared_ptr else_body_)
        : condition(condition_), body(body_), else_body(else_body_) {}

    Interpreter_result visit(Context* context) override {
        Interpreter_result conditon_res = condition->visit(context);

        if (conditon_res.is_error())
            return conditon_res;

        Interpreter_result body_res = NULL_;

        if (conditon_res.value->is_true())
            body_res = body->visit(context);
        else if (else_body != nullptr)
            body_res = else_body->visit(context);

        body_res.print = false;
        return body_res;
    }
};

struct AST_for_node : public AST_node {
    Node_shared_ptr start_node, stop_node, skip_node, body_node;

    AST_for_node(Node_shared_ptr start_node_, Node_shared_ptr stop_node_, Node_shared_ptr skip_node_, Node_shared_ptr body_node_)
        : start_node(start_node_), stop_node(stop_node_), skip_node(skip_node_), body_node(body_node_) {}

    Interpreter_result visit(Context* context) override {
        Interpreter_result start = start_node->visit(context), stop = stop_node->visit(context), skip, res;
        vector<Value_shared_ptr> arr;

        if (start.is_error())
            return start;

        if (start.value != V_NUMBER)
            return RT_Error(
                context,
                ERRORS["Expected"] + ' ' + VALUE_NAMES_MAP["NUMBER"],
                start_node->pos_start,
                start_node->pos_end);

        if (stop.is_error())
            return stop;

        if (stop.value != V_NUMBER)
            return RT_Error(
                context,
                ERRORS["Expected"] + ' ' + VALUE_NAMES_MAP["NUMBER"],
                stop_node->pos_start,
                stop_node->pos_end);

        if (skip_node == nullptr)
            skip.value = make_shared<Number_value>(1);
        else {
            skip = skip_node->visit(context);

            if (skip.is_error())
                return skip;

            if (skip.value != V_NUMBER)
                return RT_Error(
                    context,
                    ERRORS["Expected"] + ' ' + VALUE_NAMES_MAP["NUMBER"],
                    skip_node->pos_start,
                    skip_node->pos_end);
        }

        Number_value* var = static_cast<Number_value*>(start.value.get());
        double d_skip_val = stod(skip.value->value());
        double d_stop_val = stod(stop.value->value());
        double *p1 = &var->value_, *p2 = &d_stop_val;

        if (d_skip_val < 0.0)
            swap(p1, p2);

        for (; *p1 <= *p2; var->value_ += d_skip_val) {
            res = body_node->visit(context);

            if (res.is_error()) {
                if (res.error.details == ERRORS["Cannot use 'break' outside of loops"]) {
                    res.error = RT_Error();
                    break;
                }

                if (res.error.details != ERRORS["Cannot use 'contiune' outside of loops"])
                    break;

                res.error = RT_Error();
            }

            arr.push_back(res.value);
        }

        if (res.is_error())
            return res;

        res.value = make_shared<Array_value>(arr);
        res.print = false;
        return res;
    }
};

const int NO_MAX_LOOP_REPETITIONS = 1e5;

struct AST_while_node : public AST_node {
    Node_shared_ptr condition_node, body_node;

    AST_while_node(Node_shared_ptr condition_node_, Node_shared_ptr body_node_)
        : condition_node(condition_node_), body_node(body_node_) {}

    Interpreter_result visit(Context* context) override {
        Interpreter_result condition = condition_node->visit(context), res;
        vector<Value_shared_ptr> arr;

        if (condition.is_error())
            return condition;

        while (condition.value->is_true() && arr.size() < NO_MAX_LOOP_REPETITIONS) {
            res = body_node->visit(context);

            if (res.is_error()) {
                if (res.error.details == ERRORS["Cannot use 'break' outside of loops"]) {
                    res.error = RT_Error();
                    break;
                }

                if (res.error.details != ERRORS["Cannot use 'contiune' outside of loops"])
                    break;

                res.error = RT_Error();
            }

            condition = condition_node->visit(context);

            if (condition.is_error())
                return condition;

            arr.push_back(res.value);
        }

        if (res.is_error())
            return res;

        if (arr.size() >= NO_MAX_LOOP_REPETITIONS)
            return RT_Error(context, ERRORS["Infinite loop"], condition_node->pos_start, condition_node->pos_end);

        res.value = make_shared<Array_value>(arr);
        res.print = true;
        return res;
    }
};

struct AST_do_until_node : public AST_node {
    Node_shared_ptr condition_node, body_node;

    AST_do_until_node(Node_shared_ptr condition_node_, Node_shared_ptr body_node_)
        : condition_node(condition_node_), body_node(body_node_) {}

    Interpreter_result visit(Context* context) override {
        Interpreter_result condition = condition_node->visit(context), res;
        vector<Value_shared_ptr> arr;

        if (condition.is_error())
            return condition;

        do {
            res = body_node->visit(context);

            if (res.is_error()) {
                if (res.error.details == ERRORS["Cannot use 'break' outside of loops"]) {
                    res.error = RT_Error();
                    break;
                }

                if (res.error.details != ERRORS["Cannot use 'contiune' outside of loops"])
                    break;

                res.error = RT_Error();
            }

            condition = condition_node->visit(context);

            if (condition.is_error())
                return condition;

            arr.push_back(res.value);
        } while (!condition.value->is_true() && arr.size() < NO_MAX_LOOP_REPETITIONS);

        if (res.is_error())
            return res;

        if (arr.size() >= NO_MAX_LOOP_REPETITIONS)
            return RT_Error(context, ERRORS["Infinite loop"], condition_node->pos_start, condition_node->pos_end);

        res.value = make_shared<Array_value>(arr);
        res.print = true;
        return res;
    }
};

bool WRITE_INDENT = true;

struct AST_write_node : public AST_node {
    vector<Node_shared_ptr> nodes;
    Token_shared_ptr out_stream;

    AST_write_node(vector<Node_shared_ptr> nodes_, Token_shared_ptr out_stream_ = nullptr)
        : nodes(nodes_), out_stream(out_stream_) {}

    void write_cout(const string& out, const string& indent) {
        if (out.size())
            for (const auto& i : out) {
                if (WRITE_INDENT) {
                    cout << indent;
                    WRITE_INDENT = false;
                }

                cout << i;
                WRITE_INDENT = i == '\n';
            }
        else if (WRITE_INDENT) {
            cout << indent;
            WRITE_INDENT = false;
        }
    }

    void write_file(const string& out, const string& file_name, Context* context) {
        (*context->out_files[file_name].g) << out;
    }

    Interpreter_result visit(Context* context) override {
        Interpreter_result res;
        string place_holder;  // indent or file_name
        function<void(const string&, const string&, Context*)> write;

        if (out_stream == nullptr) {
            write = [&](const string& s1, const string& s2, Context* ctx) {
                write_cout(s1, s2);
            };

            for (int i = to_string(MAP_FILE_NAMES_TO_NO_LINES[PROGRAM_NAME]).size(); i > 0; i--)
                place_holder += ' ';

            place_holder += "   < ";
        } else {
            place_holder = out_stream->value();
            bool found   = false;

            for (auto& file : context->out_files)
                if (place_holder == file.first) {
                    found = true;
                    break;
                }

            if (!found)
                for (auto& file : GLOBAL_CONTEXT.out_files)
                    if (place_holder == file.first) {
                        found = true;
                        break;
                    }

            if (!found) {
                for (auto& file : context->in_files)
                    if (place_holder == file.first) {
                        found = true;
                        break;
                    }

                if (found)
                    return RT_Error(context, ERRORS["File opened for reading"], out_stream->pos_start, out_stream->pos_end);
                else
                    return RT_Error(context, ERRORS["Unknown identifier"], out_stream->pos_start, out_stream->pos_end);
            }

            write = [&](const string& s1, const string& s2, Context* ctx) {
                write_file(s1, s2, ctx);
            };
        }

        for (auto& node : nodes) {
            res = node->visit(context);

            if (res.is_error())
                return res;

            write(res.value->value(), place_holder, context);
        }

        res.print = false;
        return res;
    }
};

map<string, queue<double>> INP_STREAMS_EXTRAS;

struct AST_read_node : public AST_node {
    vector<pair<Token_shared_ptr, vector<Node_shared_ptr>>> reads;
    Token_shared_ptr file;

    AST_read_node(vector<pair<Token_shared_ptr, vector<Node_shared_ptr>>> reads_, Token_shared_ptr file_ = nullptr)
        : reads(reads_), file(file_) {}

    Node_shared_ptr make_value(const string& inp_stream_name, const string& indent, IN_stream* in) {
        if (!INP_STREAMS_EXTRAS[inp_stream_name].empty()) {
            double x = INP_STREAMS_EXTRAS[inp_stream_name].front();
            INP_STREAMS_EXTRAS[inp_stream_name].pop();
            return make_shared<AST_number_node>(make_shared<Number_token>(x));
        }

        string inp;

        if (indent.size())
            cout << indent;

        if (!((*in) >> inp))
            return make_shared<AST_node>();

        char chars_in_numbers[] = ".- 0123456789";
        bool is_number          = true;

        if (inp.size() < 0) {
            is_number = false;
            inp.push_back('\n');
        }

        for (int i = 0; i < inp.size() && is_number; i++)
            is_number = strchr(chars_in_numbers, inp[i]);

        if (!is_number)
            return make_shared<AST_string_node>(make_shared<String_token>(inp, T_STRING));

        istringstream stream(inp);
        double val;
        queue<double>* extra = &INP_STREAMS_EXTRAS[inp_stream_name];

        while (stream >> val)
            extra->push(val);

        return make_value(inp_stream_name, indent, in);
    }

    Interpreter_result evaluate(Context* context, pair<Token_shared_ptr, vector<Node_shared_ptr>> r, Node_shared_ptr val) {
        if (r.second.empty())
            return AST_variable_assign_node(r.first, val, false).visit(context);

        Node_shared_ptr node = make_shared<AST_variable_access_node>(r.first);

        for (int it = 0; it < r.second.size() - 1; it++)
            node = make_shared<AST_index_access_node>(node, r.second[it]);

        return AST_index_assign_node(r.first, node, r.second.back(), val, false).visit(context);
    }

    Interpreter_result visit(Context* context) override {
        string indent, a;
        IN_stream* in = nullptr;

        if (file == nullptr) {
            in     = cin_stream;
            indent = string(1, KEYWORDS["read"][0]);
            a      = "cin";
            for (int i = to_string(MAP_FILE_NAMES_TO_NO_LINES[PROGRAM_NAME]).size(); i > 1; i--)
                indent += ' ';

            indent += "   < ";
        } else {
            string file_name = file->value();
            a                = file_name;
            for (auto& file : context->in_files)
                if (file_name == file.first) {
                    in = &file.second;
                    break;
                }

            if (in == nullptr)
                for (auto& f : GLOBAL_CONTEXT.in_files)
                    if (file_name == f.first) {
                        in = &f.second;
                        break;
                    }

            if (in == nullptr) {
                bool found = false;

                for (auto& file : context->out_files)
                    if (file_name == file.first) {
                        found = true;
                        break;
                    }

                if (found)
                    return RT_Error(context, ERRORS["File opened for writing/appending"], file->pos_start, file->pos_end);
                else
                    return RT_Error(context, ERRORS["Unknown identifier"], file->pos_start, file->pos_end);
            }
        }

        Interpreter_result res;
        for (auto& r : reads) {
            res = evaluate(context, r, make_value(a, indent, in));

            if (res.is_error())
                return res;
        }

        res.print = false;
        return res;
    }
};

struct AST_try_node : public AST_node {
    Node_shared_ptr try_body, catch_body;

    AST_try_node(Node_shared_ptr try_body_, Node_shared_ptr catch_body_)
        : try_body(try_body_), catch_body(catch_body_) {}

    Interpreter_result visit(Context* context) override {
        Interpreter_result res = try_body->visit(context);

        if (res.is_error() && catch_body != nullptr)
            return catch_body->visit(context);

        return res.value;
    }
};

struct AST_function_definition_node : public AST_node {
    Token_shared_ptr identifier;
    Node_shared_ptr body_node;
    vector<string> args;

    AST_function_definition_node(Token_shared_ptr identifier_, Node_shared_ptr body_node_, vector<Token_shared_ptr> args_)
        : identifier(identifier_), body_node(body_node_) {
        for (auto& arg : args_)
            args.push_back(arg->value());
    }

    Interpreter_result visit(Context* context) override {
        string name = identifier == nullptr ? '<' + KEYWORDS["anonymous"] + '>' : identifier->value();
        if (identifier != nullptr)
            if (context->memory.is(name))
                if (context->memory.get(name)->is_const)
                    return RT_Error(context, ERRORS["Constant variable"], identifier->pos_start, identifier->pos_end);

        Value_shared_ptr func = make_shared<User_defined_function_value>(name, body_node, args);
        func->set_position(identifier->pos_start, identifier->pos_end);
        func->context = context;

        if (identifier != nullptr)
            context->memory.set(name, func);

        Interpreter_result r = func;
        r.print              = false;
        return r;
    }
};

struct AST_function_call_node : public AST_node {
    Node_shared_ptr name;
    vector<Node_shared_ptr> args;

    AST_function_call_node(Node_shared_ptr name_, vector<Node_shared_ptr>& args_) : name(name_), args(args_) {
        string text = LINES[name->pos_start.line].text;
        int i       = 0;
        pos_start = pos_end = name->pos_start;

        while (text[i] != '(') i++;
        pos_start.column = ++i;

        while (text[i] != ')') i++;
        pos_end.column = ++i;
    }

    Interpreter_result visit(Context* context) override {
        Interpreter_result call = name->visit(context), res;

        if (call.is_error())
            return call;

        vector<Value_shared_ptr> arg_values;

        call.value->set_position(pos_start, pos_end);
        call.value->context = context;

        for (auto& arg : args) {
            res = arg->visit(context);

            if (res.is_error())
                return res;

            res.value->set_position(arg->pos_start, arg->pos_end);
            arg_values.push_back(res.value);
        }

        call       = call.value->execute(arg_values);
        call.print = call.value != V_NULL && call.print;

        return call;
    }
};

struct AST_return_node : public AST_node {
    Node_shared_ptr ret;

    AST_return_node(Node_shared_ptr ret_) : ret(ret_) {}

    Interpreter_result visit(Context* context) override {
        Interpreter_result res = ret == nullptr ? NULL_ : ret->visit(context);

        if (res.is_error())
            return res;

        return Interpreter_result(res.value, RT_Error(
                                                 context,
                                                 ERRORS["Cannot return values outside of functions"],
                                                 ret->pos_start,
                                                 ret->pos_end));
    }
};

struct AST_break_continue_node : public AST_node {
    bool break_;

    AST_break_continue_node(bool break__, Position pos_start_, Position pos_end_)
        : break_(break__) {
        pos_start = pos_start_;
        pos_end   = pos_end_;
    }

    Interpreter_result visit(Context* context) override {
        string details = break_ ? ERRORS["Cannot use 'break' outside of loops"] : ERRORS["Cannot use 'contiune' outside of loops"];

        return RT_Error(context, details, pos_start, pos_end);
    }
};

struct AST_body_node : public AST_node {
    vector<Node_shared_ptr> body;

    AST_body_node(vector<Node_shared_ptr> body_) : body(body_) {}

    Interpreter_result visit(Context* context) override {
        Interpreter_result res;

        for (auto& expr : body) {
            res = expr->visit(context);

            if (res.is_error())
                return res;
        }

        return res.value->copy();
    }
};

Interpreter_result run(string input, IN_stream* in);

struct AST_include_node : public AST_node {
    Token_shared_ptr path;

    AST_include_node(Token_shared_ptr path_) : path(path_) {}

    Interpreter_result visit(Context* context) override {
        string str = path->value();

        if (MAP_FILE_NAMES_TO_NO_LINES[str] != 0)
            return RT_Error();

        IN_file_stream f;
        f.f = new ifstream(str);

        if (!f.f->is_open())
            return RT_Error(context, ERRORS["Cannot open file"], path->pos_start, path->pos_end);

        swap(str, GLOBAL_CONTEXT.name);

        string input;
        Interpreter_result res;

        while (f >> input) {
            res = run(input, &f);

            if (res.is_error())
                break;
        }

        GLOBAL_CONTEXT.name = str;
        res.print           = false;
        return res;
    }
};

struct AST_run_node : public AST_node {
    Token_shared_ptr path;

    AST_run_node(Token_shared_ptr path_) : path(path_) {}

    Interpreter_result visit(Context* context) override {
        initialize();
        return AST_include_node(path).visit(context);
    }
};
