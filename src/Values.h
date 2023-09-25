#pragma once
#include <map>

#include "Base.h"

struct AST_node;
struct Value;

typedef shared_ptr<Value> Value_shared_ptr;
typedef shared_ptr<AST_node> Node_shared_ptr;

struct Memory {
    map<string, Value_shared_ptr> variable_map;

    Value_shared_ptr get(string name, Memory* global = nullptr) {
        auto val = variable_map.find(name);

        if (val == variable_map.end() && global != nullptr)
            return global->get(name);

        return val->second;
    }

    void set(string name, Value_shared_ptr value) { variable_map[name] = value; }

    void erase(string name) { variable_map.erase(name); }

    bool is(string name, Memory* global = nullptr) {
        bool res = variable_map.find(name) != variable_map.end();

        if (global != nullptr && !res)
            return global->is(name);

        return res;
    }
};

struct Context {
    string name;
    Memory memory;
    map<string, IN_file_stream> in_files;
    map<string, OUT_file_stream> out_files;

    Context(string name_ = "") : name(name_) {}
};

Context GLOBAL_CONTEXT(PROGRAM_NAME);

vector<pair<string, int>> PARENT_CONTEXTS;
// pairs made of name of context and line where it is created

struct RT_Error {
    string context_name, details;
    Position pos_start, pos_end;

    RT_Error() {}
    RT_Error(Context* context_, string details_, Position pos_start_, Position pos_end_)
        : context_name(context_->name), details(details_), pos_start(pos_start_), pos_end(pos_end_) {}

    bool is_error() { return details.size(); }

    void print_error() {
        cout << ERRORS["Traceback last calls"] << ":\n";

        {
            pair<string, int> a("", -1);

            if (!PARENT_CONTEXTS.empty())
                a = PARENT_CONTEXTS.back();

            if (context_name != a.first || pos_start.line != a.second)
                cout << "   " + ERRORS["In"] + " <" << context_name << ">, " + ERRORS["line"] + ' '
                     << LINES[pos_start.line].line << '\n';
        }

        int i;

        for (i = PARENT_CONTEXTS.size() - 1; i >= 0; i--)
            cout << "   " + ERRORS["In"] + " <" << PARENT_CONTEXTS[i].first << ">, " + ERRORS["line"] + ' '
                 << LINES[PARENT_CONTEXTS[i].second].line << '\n';

        i           = 0;
        string text = LINES[pos_start.line].text;

        while (text[i] == '\t') {
            cout << ' ';
            i++;
        }

        text.erase(text.begin(), text.begin() + i);

        cout << text << '\n';

        for (i = 0; i < pos_start.column; i++)
            cout << ' ';

        for (; i < pos_end.column; i++)
            cout << '^';

        cout << '\n'
             << ERRORS["Runtime error"] << " - " << details << '\n';
    }
};

struct Interpreter_result {
    Value_shared_ptr value;
    RT_Error error;
    bool print;

    Interpreter_result(Value_shared_ptr value_, RT_Error error_ = RT_Error(), bool print_ = true) {
        value = value_;
        error = error_;
        print = print_;
    }

    Interpreter_result(bool print_ = false);
    Interpreter_result(RT_Error error_);

    bool is_error() { return error.is_error(); }
    void print_error() { error.print_error(); }
};

struct Value {
    char type;
    bool is_const;
    Position pos_start, pos_end;
    Context* context;

    Value() : type(V_NULL), is_const(false) {}

    // probably needs to be rewritten
    RT_Error illegal_operation(char value_type, Position pos_end_) {
        ostringstream oss;
        Position pos = pos_end;

        oss << ERRORS["Illegal operation"] << ": " << *VALUE_NAMES_ARRAY[type];

        if (value_type > -1) {
            oss << ", " << *VALUE_NAMES_ARRAY[value_type];
            pos = pos_end_;
        }

        return RT_Error(context, oss.str(), pos_start, pos);
    }

    void set_position(Position pos_start_, Position pos_end_) {
        pos_start = pos_start_;
        pos_end   = pos_end_;
    }

    virtual string value() { return "NULL"; }

    virtual string repr() { return "NULL"; }

    virtual bool is_true() { return false; }

    virtual Value_shared_ptr copy() {
        Value_shared_ptr x = make_shared<Value>();
        x->context         = context;
        x->set_position(pos_start, pos_end);
        return x;
    }

    Interpreter_result equals(Value_shared_ptr x);
    Interpreter_result less(Value_shared_ptr x);
    Interpreter_result less_or_equal(Value_shared_ptr x);
    Interpreter_result greater(Value_shared_ptr x);
    Interpreter_result greater_or_equal(Value_shared_ptr x);
    Interpreter_result different(Value_shared_ptr x);
    Interpreter_result and_(Value_shared_ptr x);
    Interpreter_result or_(Value_shared_ptr x);

    virtual Interpreter_result add(Value_shared_ptr x) { return illegal_operation(x->type, x->pos_end); }

    virtual Interpreter_result subtract(Value_shared_ptr x) { return illegal_operation(x->type, x->pos_end); }

    virtual Interpreter_result multiply(Value_shared_ptr x) { return illegal_operation(x->type, x->pos_end); }

    virtual Interpreter_result divide(Value_shared_ptr x) { return illegal_operation(x->type, x->pos_end); }

    virtual Interpreter_result modulo(Value_shared_ptr x) { return illegal_operation(x->type, x->pos_end); }

    virtual Interpreter_result get_at_index(Value_shared_ptr x) {
        return RT_Error(x->context, ERRORS["Accessing index of non-container value"], x->pos_start, x->pos_end);
    }

    virtual Interpreter_result set_at_index(Value_shared_ptr idx, Value_shared_ptr x, bool is_const_) {
        return RT_Error(idx->context, ERRORS["Accessing index of non-container value"], idx->pos_start, idx->pos_end);
    }

    virtual Interpreter_result execute(vector<Value_shared_ptr> args) { return illegal_operation(-1, pos_end); }

    virtual ~Value() = default;
};

bool operator==(const Value_shared_ptr& v, const char type) {
    return v->type == type;
}

bool operator!=(const Value_shared_ptr& v, const char type) {
    return v->type != type;
}

Value_shared_ptr NULL_ = make_shared<Value>();

Interpreter_result::Interpreter_result(bool print_) {
    value = NULL_;
    error = RT_Error();
    print = print_;
}

Interpreter_result::Interpreter_result(RT_Error error_) {
    value = NULL_;
    error = error_;
    print = false;
}

struct Number_value : public Value {
    double value_;

    Number_value(double value_ = 0.0, bool is_const_ = false) : value_(value_) {
        type     = V_NUMBER;
        is_const = is_const_;
    }

    string value() override { return double_to_string(value_); }

    string repr() override { return double_to_string(value_); }

    bool is_true() override { return value_; }

    Value_shared_ptr copy() override {
        Value_shared_ptr x = make_shared<Number_value>(value_, is_const);
        x->context         = context;
        x->set_position(pos_start, pos_end);
        return x;
    }

    Interpreter_result add(Value_shared_ptr x) override {
        if (x != V_NUMBER)
            return illegal_operation(x->type, x->pos_end);

        return Interpreter_result(make_shared<Number_value>(value_ + stod(x->value())));
    }

    Interpreter_result subtract(Value_shared_ptr x) override {
        if (x != V_NUMBER)
            return illegal_operation(x->type, x->pos_end);

        return Interpreter_result(make_shared<Number_value>(value_ - stod(x->value())));
    }

    Interpreter_result multiply(Value_shared_ptr x) override {
        if (x != V_NUMBER)
            return illegal_operation(x->type, x->pos_end);

        return Interpreter_result(make_shared<Number_value>(value_ * stod(x->value())));
    }

    Interpreter_result divide(Value_shared_ptr x) override {
        if (x != V_NUMBER)
            return illegal_operation(x->type, x->pos_end);

        double v = stod(x->value());

        if (v == 0.0)
            return RT_Error(context, ERRORS["Division by 0"], x->pos_start, x->pos_end);

        return Interpreter_result(make_shared<Number_value>(value_ / v));
    }

    Interpreter_result modulo(Value_shared_ptr x) override {
        if (x != V_NUMBER)
            return illegal_operation(x->type, x->pos_end);

        double v = stod(x->value());

        if (v == 0.0)
            return RT_Error(context, ERRORS["Division by 0"], x->pos_start, x->pos_end);

        return Interpreter_result(make_shared<Number_value>(fmod(value_, v)));
    }
};

Interpreter_result Value::equals(Value_shared_ptr x) {
    if (x != type)
        return Interpreter_result(make_shared<Number_value>(0));

    return Interpreter_result(make_shared<Number_value>(value() == x->value()));
}

Interpreter_result Value::different(Value_shared_ptr x) {
    if (x != type)
        return Interpreter_result(make_shared<Number_value>(1));

    return Interpreter_result(make_shared<Number_value>(value() != x->value()));
}

Interpreter_result Value::less(Value_shared_ptr x) {
    if (x != type)
        return illegal_operation(x->type, x->pos_end);

    string v1 = value(), v2 = x->value();
    bool v;
    if (type == V_NUMBER)
        v = stod(v1) < stod(v2);
    else
        v = v1 < v2;

    return Interpreter_result(make_shared<Number_value>(v));
}

Interpreter_result Value::less_or_equal(Value_shared_ptr x) {
    if (x != type)
        return illegal_operation(x->type, x->pos_end);

    string v1 = value(), v2 = x->value();
    bool v;
    if (type == V_NUMBER)
        v = stod(v1) <= stod(v2);
    else
        v = v1 <= v2;

    return Interpreter_result(make_shared<Number_value>(v));
}

Interpreter_result Value::greater(Value_shared_ptr x) {
    if (x != type)
        return illegal_operation(x->type, x->pos_end);

    string v1 = value(), v2 = x->value();
    bool v;
    if (type == V_NUMBER)
        v = stod(v1) > stod(v2);
    else
        v = v1 > v2;

    return Interpreter_result(make_shared<Number_value>(v));
}

Interpreter_result Value::greater_or_equal(Value_shared_ptr x) {
    if (x != type)
        return illegal_operation(x->type, x->pos_end);

    string v1 = value(), v2 = x->value();
    bool v;
    if (type == V_NUMBER)
        v = stod(v1) >= stod(v2);
    else
        v = v1 >= v2;

    return Interpreter_result(make_shared<Number_value>(v));
}

Interpreter_result Value::and_(Value_shared_ptr x) {
    return Interpreter_result(make_shared<Number_value>(is_true() && x->is_true()));
}

Interpreter_result Value::or_(Value_shared_ptr x) {
    return Interpreter_result(make_shared<Number_value>(is_true() || x->is_true()));
}

struct Char_value : public Value {
    char value_;

    Char_value(char char_, bool is_const_ = false) : value_(char_) {
        type     = V_CHAR;
        is_const = is_const_;
    }

    string value() override {
        return string(1, value_);
    }

    string repr() override {
        ostringstream oss;
        oss << '\'';

        map<char, char> escaped_chars_rev;

        for (auto& c : ESCAPED_CHARS)
            escaped_chars_rev[c.second] = c.first;

        if (escaped_chars_rev.find(value_) != escaped_chars_rev.end())
            oss << '\\' << escaped_chars_rev[value_];
        else
            oss << value_;

        oss << '\'';
        return oss.str();
    }

    bool is_true() override { return value_; }

    Value_shared_ptr copy() override {
        Value_shared_ptr x = make_shared<Char_value>(value_, is_const);
        x->context         = context;
        x->set_position(pos_start, pos_end);
        return x;
    }

    Interpreter_result subtract(Value_shared_ptr x) override {
        int v = value_;

        if (x == V_NUMBER)
            v -= stoi(x->value());
        else
            return illegal_operation(x->type, x->pos_end);

        if (v < 0 || v > 255) {
            ostringstream oss;
            oss << ERRORS["Out of bounds"] << ": [" << 0 << ", " << 255 << "] - " << v;
            return RT_Error(context, oss.str(), pos_start, x->pos_end);
        }

        return Interpreter_result(make_shared<Char_value>(v));
    }

    Interpreter_result add(Value_shared_ptr x) override;
    Interpreter_result multiply(Value_shared_ptr x) override;
};

struct Array_value : public Value {
    vector<Value_shared_ptr> arr;

    Array_value(vector<Value_shared_ptr>& arr_, bool is_const_ = false) : arr(arr_) {
        type     = V_ARRAY;
        is_const = is_const_;
    }

    string value() override {
        if (arr.empty())
            return "[]";

        string str;

        for (auto& i : arr)
            str += i->repr() + ", ";

        str.erase(str.end() - 2);
        return "[ " + str + ']';
    }

    string repr() override { return value(); }

    bool is_true() override { return !arr.empty(); }

    Value_shared_ptr copy() override {
        Value_shared_ptr x = make_shared<Array_value>(arr, is_const);
        x->context         = context;
        x->set_position(pos_start, pos_end);
        return x;
    }

    Interpreter_result add(Value_shared_ptr x) override {
        vector<Value_shared_ptr> new_arr = arr;

        if (x != V_ARRAY) {
            new_arr.push_back(x->copy());
            return Interpreter_result(make_shared<Array_value>(new_arr));
        }

        vector<Value_shared_ptr> add_arr = static_cast<Array_value*>(x.get())->arr;

        for (auto& add : add_arr)
            new_arr.push_back(add->copy());

        return Interpreter_result(make_shared<Array_value>(new_arr));
    }

    Interpreter_result subtract(Value_shared_ptr x) override {
        if (x != V_NUMBER)
            return illegal_operation(x->type, x->pos_end);

        auto check = check_index(x, false);

        if (check.second.is_error())
            return check.second;

        vector<Value_shared_ptr> new_arr = arr;

        new_arr.erase(new_arr.begin() + check.first);

        return Interpreter_result(make_shared<Array_value>(new_arr));
    }

    Interpreter_result multiply(Value_shared_ptr x) override {
        if (x != V_NUMBER)
            return illegal_operation(x->type, x->pos_end);

        vector<Value_shared_ptr> new_arr;

        for (int i = stoi(x->value()); i > 0; i--)
            new_arr.insert(new_arr.end(), arr.begin(), arr.end());

        return Interpreter_result(make_shared<Array_value>(new_arr));
    }

    pair<int, RT_Error> check_index(Value_shared_ptr x, bool assign) {
        if (x != V_NUMBER)
            return make_pair(-1, RT_Error(context, ERRORS["Expected"] + ' ' + VALUE_NAMES_MAP["NUMBER"], x->pos_start, x->pos_end));

        int idx = stoi(x->value());
        idx += (idx < 0) * arr.size();

        if (assign)
            if (idx == arr.size())
                // "a" has no specific meaning, just need an error
                return make_pair(idx, RT_Error(&GLOBAL_CONTEXT, "a", Position(), Position()));

        if (arr.empty())
            return make_pair(-1, RT_Error(context, ERRORS["Empty container"], x->pos_start, x->pos_end));

        if (idx < 0 || idx >= arr.size()) {
            ostringstream oss;
            oss << ERRORS["Out of bounds"] << ": [" << -int(arr.size())
                << ", " << arr.size() - 1 << "] - " << idx;
            return make_pair(-1, RT_Error(x->context, oss.str(), x->pos_start, x->pos_end));
        }

        return make_pair(idx, RT_Error());
    }

    Interpreter_result get_at_index(Value_shared_ptr index) override {
        auto check = check_index(index, false);

        if (check.second.is_error())
            return check.second;

        return arr[check.first];
    }

    Interpreter_result set_at_index(Value_shared_ptr index, Value_shared_ptr x, bool is_const_) override {
        auto check = check_index(index, true);

        if (check.second.is_error())
            if (check.second.details != "a")
                return check.second;

        if (is_const)
            return RT_Error(context, ERRORS["Index cannnot be constant"], index->pos_start, index->pos_end);

        if (check.second.is_error())
            arr.push_back(x);
        else
            arr[check.first] = x;

        Interpreter_result res = x;
        res.print              = false;
        return res;
    }
};

struct String_value : public Value {
    string value_;

    String_value(string str, bool is_const_ = false) : value_(str) {
        type     = V_STRING;
        is_const = is_const_;
    }

    string value() override { return value_; }

    string repr() override {
        ostringstream oss;
        oss << '"';

        map<char, char> escaped_chars_rev;

        for (auto& c : ESCAPED_CHARS)
            escaped_chars_rev[c.second] = c.first;

        for (auto& c : value_)
            if (escaped_chars_rev.find(c) != escaped_chars_rev.end())
                oss << '\\' << escaped_chars_rev[c];
            else
                oss << c;

        oss << '"';
        return oss.str();
    }

    bool is_true() override { return !value_.empty(); }

    Value_shared_ptr copy() override {
        Value_shared_ptr x = make_shared<String_value>(value_, is_const);
        x->context         = context;
        x->set_position(pos_start, pos_end);
        return x;
    }

    Interpreter_result add(Value_shared_ptr x) override {
        if (x == V_CHAR || x == V_STRING)
            return Interpreter_result(make_shared<String_value>(value_ + x->value()));

        if (x != V_NUMBER)
            return illegal_operation(x->type, x->pos_end);

        auto check = check_index(x, false);

        if (check.second.is_error())
            return check.second;

        return Interpreter_result(make_shared<String_value>(value_.c_str() + check.first));
    }

    Interpreter_result multiply(Value_shared_ptr x) override {
        if (x != V_NUMBER)
            return illegal_operation(x->type, x->pos_end);

        string new_str;

        for (int i = stoi(x->value()); i > 0; i--)
            new_str.insert(new_str.end(), value_.begin(), value_.end());

        return Interpreter_result(make_shared<String_value>(new_str));
    }

    pair<int, RT_Error> check_index(Value_shared_ptr x, bool assign) {
        if (x != V_NUMBER)
            return make_pair(-1, RT_Error(context, ERRORS["Expected"] + ' ' + VALUE_NAMES_MAP["NUMBER"], x->pos_start, x->pos_end));

        int idx = stoi(x->value());
        idx += (idx < 0) * value_.size();

        if (assign)
            if (idx == value_.size())
                // "a" has no specific meaning, just need an error
                return make_pair(idx, RT_Error(&GLOBAL_CONTEXT, "a", Position(), Position()));

        if (value_.empty())
            return make_pair(-1, RT_Error(context, ERRORS["Empty container"], x->pos_start, x->pos_end));

        if (idx < 0 || idx >= value_.size()) {
            ostringstream oss;
            oss << ERRORS["Out of bounds"] << ": [" << -int(value_.size())
                << ", " << value_.size() - 1 << "] - " << idx;
            return make_pair(-1, RT_Error(x->context, oss.str(), x->pos_start, x->pos_end));
        }

        return make_pair(idx, RT_Error());
    }

    Interpreter_result get_at_index(Value_shared_ptr index) override {
        auto check = check_index(index, false);

        if (check.second.is_error())
            return check.second;

        return Interpreter_result(make_shared<Char_value>(value_[check.first]));
    }

    Interpreter_result set_at_index(Value_shared_ptr index, Value_shared_ptr x, bool is_const_) override {
        if (x != V_CHAR)
            return RT_Error(context, ERRORS["Expected"] + ' ' + VALUE_NAMES_MAP["CHAR"], x->pos_start, x->pos_end);

        auto check = check_index(index, true);

        if (check.second.is_error())
            if (check.second.details != "a")
                return check.second;

        if (is_const)
            return RT_Error(context, ERRORS["Index cannnot be constant"], index->pos_start, index->pos_end);

        if (check.second.is_error())
            value_.push_back(x->value()[0]);
        else
            value_[check.first] = x->value()[0];

        Interpreter_result res = value_[check.first];
        res.print              = false;
        return res;
    }
};

Interpreter_result Char_value::add(Value_shared_ptr x) {
    if (x == V_STRING)
        return Interpreter_result(make_shared<String_value>(value() + x->value()));

    int v = value_;

    if (x == V_NUMBER)
        v += stoi(x->value());
    else
        return illegal_operation(x->type, x->pos_end);

    if (v < 0 || v > 255) {
        ostringstream oss;
        oss << ERRORS["Out of bounds"] << ": [" << 0 << ", " << 255 << "] - " << v;
        return RT_Error(context, oss.str(), pos_start, x->pos_end);
    }

    return Interpreter_result(make_shared<Char_value>(v));
}

Interpreter_result Char_value::multiply(Value_shared_ptr x) {
    return Interpreter_result(make_shared<String_value>(value())->multiply(x));
}

struct Dictionary_value : public Value {
    map<string, Value_shared_ptr> m;

    Dictionary_value() {
        m = map<string, Value_shared_ptr>();
    }

    Dictionary_value(map<string, Value_shared_ptr>& m_, bool is_const_ = false) : m(m_) {
        type     = V_DICTIONARY;
        is_const = is_const_;
    }

    string value() override {
        if (m.empty())
            return "{}";

        string str;

        for (auto& i : m) {
            str += i.first + " : " + i.second->repr() + ", ";
        }

        str.erase(str.end() - 2);
        return "{ " + str + '}';
    }

    string repr() override { return value(); }

    bool is_true() override { return !m.empty(); }

    Value_shared_ptr copy() override {
        Value_shared_ptr x = make_shared<Dictionary_value>(m, is_const);
        x->context         = context;
        x->set_position(pos_start, pos_end);
        return x;
    }

    Interpreter_result subtract(Value_shared_ptr x) override {
        return Interpreter_result(make_shared<Number_value>(m.erase(x->value())));
    }

    pair<string, RT_Error> check_index(Value_shared_ptr x) {
        switch (x->type) {
            case V_NUMBER: break;
            case V_CHAR: break;
            case V_STRING: break;
            default: {
                ostringstream details;
                details << ERRORS["Expected"] << ' ' << VALUE_NAMES_MAP["NUMBER"] << ", " << VALUE_NAMES_MAP["CHAR"]
                        << ", " << VALUE_NAMES_MAP["STRING"];

                return make_pair("", RT_Error(x->context, details.str(), x->pos_start, x->pos_end));
            }
        }

        return make_pair(x->repr(), RT_Error());
    }

    Interpreter_result get_at_index(Value_shared_ptr index) override {
        auto check = check_index(index);

        if (check.second.is_error())
            return check.second;

        if (m.find(check.first) == m.end())
            return Interpreter_result(false);

        return m[check.first];
    }

    Interpreter_result set_at_index(Value_shared_ptr index, Value_shared_ptr x, bool is_const_) override {
        auto check = check_index(index);

        if (check.second.is_error())
            return check.second;

        if (is_const)
            return RT_Error(context, ERRORS["Index cannnot be constant"], index->pos_start, index->pos_end);

        m[check.first]         = x;
        Interpreter_result res = x;
        res.print              = false;
        return res;
    }
};

struct Function_base_value : public Value {
    string name;
    vector<string> arg_names;

    Context make_context() {
        Context c(name);

        if (!PARENT_CONTEXTS.empty()) {
            auto p = PARENT_CONTEXTS.back();

            if (p.first == context->name && p.second == pos_start.line)
                return c;
        }

        PARENT_CONTEXTS.emplace_back(context->name, pos_start.line);
        return c;
    }

    Interpreter_result check_args(const vector<Value_shared_ptr>& args) {
        if (args.size() < arg_names.size())
            return RT_Error(context, ERRORS["Too few arguments"], pos_start, pos_end);

        if (args.size() > arg_names.size())
            return RT_Error(context, ERRORS["Too many arguments"], pos_start, pos_end);

        return Interpreter_result(false);
    }

    void set_arg_values(vector<Value_shared_ptr>& args, Context* new_context) {
        for (int idx = 0; idx < args.size(); idx++) {
            args[idx]->context = new_context;
            new_context->memory.set(arg_names[idx], args[idx]);
        }
    }

    string value() override {
        string str = VALUE_NAMES_MAP["FUNCTION"] + ' ' + name + " (";

        for (auto& i : arg_names)
            str += i + ", ";

        if (arg_names.size()) {
            str.pop_back();
            str.pop_back();
        }

        return str + ')';
    }

    string repr() override { return value(); }
};

struct AST_node {
    Position pos_start, pos_end;

    virtual Interpreter_result visit(Context* context) {
        return Interpreter_result(false);
    }
};

typedef shared_ptr<AST_node> Node_shared_ptr;

struct User_defined_function_value : public Function_base_value {
    Node_shared_ptr body;

    User_defined_function_value(string name_, Node_shared_ptr body_, vector<string> arg_names_)
        : body(body_) {
        name      = name_;
        arg_names = arg_names_;
        type      = V_FUNCTION;
        is_const  = true;
    }

    Value_shared_ptr copy() override { return make_shared<User_defined_function_value>(name, body, arg_names); }

    Interpreter_result execute(vector<Value_shared_ptr> args) override {
        Context new_context = make_context();

        Interpreter_result res = check_args(args);

        if (res.is_error())
            return res;

        set_arg_values(args, &new_context);

        res = body->visit(&new_context);

        if (res.is_error())
            if (res.error.details == ERRORS["Cannot return values outside of functions"])
                return res.value;

        res.value = NULL_;

        return res;
    }
};

const char exit_error[] = "--exit";
void initialize();

struct Predefined_function_value : public Function_base_value {
    Predefined_function_value(string name_, vector<string> arg_names_) {
        name      = name_;
        arg_names = arg_names_;
        type      = V_FUNCTION;
        is_const  = true;
    }

    Value_shared_ptr copy() override { return make_shared<Predefined_function_value>(name, arg_names); }

    Interpreter_result execute(vector<Value_shared_ptr> args) override {
        if (name == BUILT_INS["exit"]) {
            string details = exit_error;

            if (args.empty())
                details += " 0";
            else
                for (auto& a : args)
                    details += ' ' + a->value();

            return RT_Error(context, details, Position(), Position());
        }

        if (name == BUILT_INS["reset"]) {
            initialize();
            return Interpreter_result(false);
        }

        Interpreter_result rez = check_args(args);

        if (rez.is_error())
            return rez;

        Context new_context = make_context();

        set_arg_values(args, &new_context);

        unordered_map<string, int> map = {
            { BUILT_INS["int"], 1 },
            { BUILT_INS["size"], 2 },
            { BUILT_INS["type"], 3 },
            { BUILT_INS["locals"], 4 },
            { BUILT_INS["number"], 5 },
            { BUILT_INS["string"], 6 },
            { BUILT_INS["globals"], 7 },
            { BUILT_INS["open_file"], 8 },
            { BUILT_INS["close_file"], 9 },
            { BUILT_INS["global_assign"], 10 },
            { BUILT_INS["get_dict_keys"], 11 },
        };

        switch (map[name]) {
            case 0: return RT_Error(context, "unknown predefined function", pos_start, pos_end);
            case 1: return execute_int(args[0]);
            case 2: return execute_size(args[0]);
            case 3: return execute_type(args[0]);
            case 4: return execute_locals();
            case 5: return execute_number(args[0]);
            case 6: return execute_string(args[0]);
            case 7: return execute_globals();
            case 8: return execute_open_file(args[0], args[1], args[2]);
            case 9: return execute_close_file(args[0]);
            case 10: return execute_global_assign(args[0], args[1]);
            case 11: return execute_get_dict_keys(args[0]);
        }

        return Interpreter_result(false);
    }

    Interpreter_result execute_int(Value_shared_ptr value) {
        if (value != V_NUMBER)
            return RT_Error(context, ERRORS["Expected"] + ' ' + VALUE_NAMES_MAP["NUMBER"], value->pos_start, value->pos_end);

        return Interpreter_result(make_shared<Number_value>(stoi(value->value())));
    }

    Interpreter_result execute_size(Value_shared_ptr array) {
        if (array == V_ARRAY)
            return Interpreter_result(make_shared<Number_value>(static_cast<Array_value*>(array.get())->arr.size()));

        if (array == V_STRING)
            return Interpreter_result(make_shared<Number_value>(static_cast<String_value*>(array.get())->value_.size()));

        ostringstream oss;
        oss << ERRORS["Expected"] << ' ' << VALUE_NAMES_MAP["ARRAY"] << ", " << VALUE_NAMES_MAP["STRING"];

        return RT_Error(context, oss.str(), array->pos_start, array->pos_end);
    }

    Interpreter_result execute_type(Value_shared_ptr value) {
        return Interpreter_result(make_shared<String_value>(*VALUE_NAMES_ARRAY[value->type]));
    }

    Interpreter_result execute_locals() {
        Dictionary_value dict;

        for (auto& var : context->memory.variable_map)
            dict.m['"' + var.first + '"'] = var.second->copy();

        for (auto& file : context->in_files)
            dict.m['"' + file.first + '"'] =
                make_shared<String_value>(ERRORS["File"] + " -> " + BUILT_INS["file mode read"] + file.second.path);

        for (auto& file : context->out_files)
            dict.m['"' + file.first + '"'] =
                make_shared<String_value>(ERRORS["File"] + " -> " + *file.second.mode + ' ' + file.second.path);

        return Interpreter_result(make_shared<Dictionary_value>(dict));
    }

    Interpreter_result execute_number(Value_shared_ptr value) {
        return Interpreter_result(make_shared<Number_value>(stod(value->value())));
    }

    Interpreter_result execute_string(Value_shared_ptr value) {
        return Interpreter_result(make_shared<String_value>(value->value()));
    }

    Interpreter_result execute_globals() {
        Dictionary_value dict;

        for (auto& var : GLOBAL_CONTEXT.memory.variable_map)
            dict.m['"' + var.first + '"'] = var.second->copy();

        for (auto& file : context->in_files)
            dict.m['"' + file.first + '"'] =
                make_shared<String_value>(ERRORS["File"] + " -> " + BUILT_INS["file mode read"] + file.second.path);

        for (auto& file : context->out_files)
            dict.m['"' + file.first + '"'] =
                make_shared<String_value>(ERRORS["File"] + " -> " + *file.second.mode + ' ' + file.second.path);

        return Interpreter_result(make_shared<Dictionary_value>(dict));
    }

    Interpreter_result execute_open_file(Value_shared_ptr name, Value_shared_ptr path, Value_shared_ptr mode) {
        if (name != V_STRING)
            return RT_Error(context, ERRORS["Expected"] + ' ' + VALUE_NAMES_MAP["STRING"], name->pos_start, name->pos_end);

        if (path != V_STRING)
            return RT_Error(context, ERRORS["Expected"] + ' ' + VALUE_NAMES_MAP["STRING"], path->pos_start, path->pos_end);

        if (mode != V_STRING)
            return RT_Error(context, ERRORS["Expected"] + ' ' + VALUE_NAMES_MAP["STRING"], mode->pos_start, mode->pos_end);

        string file_name = name->value();

        bool is_identifier = strchr(IDENTIFIER_CHARS, file_name[0]) && (file_name[0] < '0' || file_name[0] > '9');

        for (int i = 1; i < file_name.size() && is_identifier; i++)
            is_identifier = strchr(IDENTIFIER_CHARS, file_name[i]);

        if (!is_identifier)
            return RT_Error(context, ERRORS["Not an identifier"], name->pos_start, name->pos_end);

        for (auto& i : context->in_files)
            if (file_name == i.first)
                return RT_Error(context, ERRORS["File already opened"], name->pos_start, name->pos_end);

        for (auto& i : context->out_files)
            if (file_name == i.first)
                return RT_Error(context, ERRORS["File already opened"], name->pos_start, name->pos_end);

        if (context->memory.is(file_name))
            if (context->memory.get(file_name)->is_const)
                return RT_Error(context, ERRORS["Constant variable"], name->pos_start, name->pos_end);
            else
                context->memory.erase(file_name);

        string m = mode->value();

        if (m == BUILT_INS["file mode read"]) {
            context->in_files[file_name].f = new ifstream(path->value());

            if (!context->in_files[file_name].f->is_open()) {
                context->in_files.erase(file_name);
                return RT_Error(context, ERRORS["Cannot open file"], path->pos_start, path->pos_end);
            }
        } else if (m == BUILT_INS["file mode write"]) {
            context->out_files[file_name].g    = new ofstream(path->value(), fstream::out);
            context->out_files[file_name].mode = &BUILT_INS["file mode write"];

            if (!context->out_files[file_name].g->is_open()) {
                context->out_files.erase(file_name);
                return RT_Error(context, ERRORS["Cannot open file"], path->pos_start, path->pos_end);
            }
        } else if (m == BUILT_INS["file mode append"]) {
            context->out_files[file_name].g    = new ofstream(path->value(), fstream::app);
            context->out_files[file_name].mode = &BUILT_INS["file mode append"];

            if (!context->out_files[file_name].g->is_open()) {
                context->out_files.erase(file_name);
                return RT_Error(context, ERRORS["Cannot open file"], path->pos_start, path->pos_end);
            }
        } else
            return RT_Error(context, ERRORS["Unknown file mode"], mode->pos_start, mode->pos_end);

        return Interpreter_result(false);
    }

    Interpreter_result execute_close_file(Value_shared_ptr identifier) {
        if (identifier != V_STRING)
            return RT_Error(
                context,
                ERRORS["Expected"] + ' ' + VALUE_NAMES_MAP["STRING"],
                identifier->pos_start,
                identifier->pos_end);

        bool found             = false;
        string file_identifier = identifier->value();

        for (auto it = context->in_files.begin(); it != context->in_files.end() && !found; it++)
            found = file_identifier == it->first;

        if (found) {
            context->in_files.erase(file_identifier);
            return Interpreter_result(false);
        }

        for (auto it = context->out_files.begin(); it != context->out_files.end() && !found; it++)
            found = file_identifier == it->first;

        if (found) {
            context->out_files.erase(file_identifier);
            return Interpreter_result(false);
        }

        return RT_Error(context, ERRORS["Unknown identifier"], identifier->pos_start, identifier->pos_end);
    }

    Interpreter_result execute_global_assign(Value_shared_ptr name, Value_shared_ptr value) {
        if (name != V_STRING)
            return RT_Error(context, ERRORS["Expected"] + ' ' + VALUE_NAMES_MAP["STRING"], name->pos_start, name->pos_end);

        string variable_name = name->value();

        bool is_identifier = strchr(IDENTIFIER_CHARS, variable_name[0]) && (variable_name[0] < '0' || variable_name[0] > '9');

        for (int i = 1; i < variable_name.size() && is_identifier; i++)
            is_identifier = strchr(IDENTIFIER_CHARS, variable_name[i]);

        if (!is_identifier)
            return RT_Error(context, ERRORS["Not an identifier"], name->pos_start, name->pos_end);

        for (auto& i : context->in_files)
            if (variable_name == i.first)
                return RT_Error(context, ERRORS["File already opened"], name->pos_start, name->pos_end);

        for (auto& i : context->out_files)
            if (variable_name == i.first)
                return RT_Error(context, ERRORS["File already opened"], name->pos_start, name->pos_end);

        if (context->memory.is(variable_name))
            if (context->memory.get(variable_name)->is_const)
                return RT_Error(context, ERRORS["Constant variable"], name->pos_start, name->pos_end);

        context->memory.set(variable_name, value->copy());
        Interpreter_result res(context->memory.get(variable_name));
        res.print = false;
        return res;
    }

    Interpreter_result execute_get_dict_keys(Value_shared_ptr dict) {
        if (dict != V_DICTIONARY)
            return RT_Error(context, ERRORS["Expected"] + ' ' + VALUE_NAMES_MAP["DICTIONARY"], dict->pos_start, dict->pos_end);

        vector<Value_shared_ptr> arr;
        for (auto& i : static_cast<Dictionary_value*>(dict.get())->m)
            arr.push_back(make_shared<String_value>(i.first));

        return Interpreter_result(make_shared<Array_value>(arr));
    }
};
