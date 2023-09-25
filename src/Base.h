#pragma once

#include <cmath>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <vector>

using namespace std;

// constants
const char PROGRAM_NAME[] = "main";
const char COMMENT_CHAR   = '$';

unordered_map<char, char> ESCAPED_CHARS = {
    { '0', '\0' }, { 'a', '\a' }, { 'b', '\b' }, { 'f', '\f' }, { 'n', '\n' }, { 'r', '\r' }, { 't', '\t' }, { 'v', '\v' }
};

const char IDENTIFIER_CHARS[] =
    "_0123456789AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz";

unordered_map<string, string> ERRORS = {
    { "File", "" },
    { "In", "" },
    { "line", "" },
    { "Expected", "" },
    { "Lexing error", "" },
    { "Runtime error", "" },
    { "Division by 0", "" },
    { "Out of bounds", "" },
    { "Parsing error", "" },
    { "Invalid syntax", "" },
    { "Empty container", "" },
    { "Constant variable", "" },
    { "Illegal operation", "" },
    { "Unknown identifier", "" },
    { "Use \"\" for string", "" },
    { "Expression expected", "" },
    { "Identifier expected", "" },
    { "Traceback last calls", "" },
    { "Incorrect instruction", "" },
    { "Index cannnot be constant", "" },
    { "Accessing index of non-container value", "" },
    { "Assignment expected", "" },
    { "Infinite loop", "" },
    { "Too many arguments", "" },
    { "Too few arguments", "" },
    { "Program quit with code", "" },
    { "Not an identifier", "" },
    { "Unknown file mode", "" },
    { "Cannot open file", "" },
    { "File already opened", "" },
    { "File opened for reading", "" },
    { "File opened for writing/appending", "" },
    { "Cannot reference files", "" },
    { "Cannot return values outside of functions", "" },
    { "Cannot use 'break' outside of loops", "" },
    { "Cannot use 'contiune' outside of loops", "" },
    { "End of line expected", "" },
    { "Reached end of file and not all bodies are closed", "" }
};

unordered_map<string, string> KEYWORDS = {
    { "or", "" },
    { "and", "" },
    { "const", "" },
    { "if", "" },
    { "then", "" },
    { "else", "" },
    { "end", "" },
    { "for", "" },
    { "execute", "" },
    { "while", "" },
    { "do", "" },
    { "until", "" },
    { "print", "" },
    { "read", "" },
    { "try", "" },
    { "catch", "" },
    { "function", "" },
    { "return", "" },
    { "continue", "" },
    { "break", "" },
    { "include", "" },
    { "run", "" }
};

unordered_map<string, string> BUILT_INS = {
    { "TRUE", "" },
    { "FALSE", "" },
    { "NULL", "" },
    { "anonymous", "" },
    { "exit", "" },
    { "int", "" },
    { "reset", "" },
    { "locals", "" },
    { "globals", "" },
    { "array", "" },
    { "string", "" },
    { "number", "" },
    { "type", "" },
    { "size", "" },
    { "global_assign", "" },
    { "get_dict_keys", "" },
    { "open_file", "" },
    { "value", "" },
    { "name", "" },
    { "path", "" },
    { "mode", "" },
    { "dictionary", "" },
    { "file mode read", "" },
    { "file mode write", "" },
    { "file mode append", "" },
    { "close_file", "" },
    { "identifier", "" }
};

// tokens
const char T_NULL                = 0,
           T_NUMBER              = 1,
           T_PLUS                = 2,
           T_MINUS               = 3,
           T_MULTIPLY            = 4,
           T_DIVIDE              = 5,
           T_MODULO              = 6,
           T_STRING              = 7,
           T_END_LINE            = 8,
           T_AND                 = 9,
           T_OR                  = 10,
           T_CHAR                = 11,
           T_KEYWORD             = 12,
           T_IDENTIFIER          = 13,
           T_ROUND_BRACKET_LEFT  = 14,
           T_ROUND_BRACKET_RIGHT = 15,
           T_BOX_BRACKET_LEFT    = 16,
           T_BOX_BRACKET_RIGHT   = 17,
           T_CURLY_BRACKET_LEFT  = 18,
           T_CURLY_BRACKET_RIGHT = 19,
           T_COMMA               = 20,
           T_EQUALS              = 21,
           T_LESS                = 22,
           T_LESS_EQUAL          = 23,
           T_GREATER             = 24,
           T_GREATER_EQUAL       = 25,
           T_DIFFERENT           = 26,
           T_ASSIGNMENT          = 27,
           T_COLON               = 28;

unordered_map<char, char> OPERATIONS = {
    { '+', T_PLUS },
    { '-', T_MINUS },
    { '/', T_DIVIDE },
    { '%', T_MODULO },
    { '*', T_MULTIPLY },
    { '=', T_EQUALS },
    { '(', T_ROUND_BRACKET_LEFT },
    { ')', T_ROUND_BRACKET_RIGHT },
    { '[', T_BOX_BRACKET_LEFT },
    { ']', T_BOX_BRACKET_RIGHT },
    { '{', T_CURLY_BRACKET_LEFT },
    { '}', T_CURLY_BRACKET_RIGHT },
    { ',', T_COMMA },
    { ':', T_COLON }
};

// value types
const char V_NULL       = 0,
           V_NUMBER     = 1,
           V_CHAR       = 2,
           V_ARRAY      = 3,
           V_STRING     = 4,
           V_DICTIONARY = 5,
           V_FUNCTION   = 6;

unordered_map<string, string> VALUE_NAMES_MAP = {
    { "NULL", "" },
    { "NUMBER", "" },
    { "CHAR", "" },
    { "ARRAY", "" },
    { "STRING", "" },
    { "DICTIONARY", "" },
    { "FUNCTION", "" }
};

const string *VALUE_NAMES_ARRAY[] = {
    &VALUE_NAMES_MAP["NULL"], &VALUE_NAMES_MAP["NUMBER"],
    &VALUE_NAMES_MAP["CHAR"], &VALUE_NAMES_MAP["ARRAY"],
    &VALUE_NAMES_MAP["STRING"], &VALUE_NAMES_MAP["DICTIONARY"],
    &VALUE_NAMES_MAP["FUNCTION"]
};

unordered_map<string, int> MAP_FILE_NAMES_TO_NO_LINES;

struct Line {
    string file_name, text;
    int line;

    Line(string file_name_, string text_) {
        file_name = file_name_;
        text      = text_;
        line      = ++MAP_FILE_NAMES_TO_NO_LINES[file_name];
    }
};

vector<Line> LINES;

struct Position {
    int column, line;

    void advance() { column++; }

    Position(int line_ = 0, int column_ = -1) : column(column_), line(line_) {}
};

struct Error {
    string name, details;
    Position pos_start, pos_end;

    Error() {}
    Error(string name_, string details_, Position pos_start_, Position pos_end_)
        : name(name_), details(details_), pos_start(pos_start_), pos_end(pos_end_) {}

    bool is_error() { return name.size() + details.size(); }

    void print_error() {
        if (!is_error())
            return;

        cout << ERRORS["File"] << " <" << LINES[pos_start.line].file_name << ">, "
             << ERRORS["line"] << ' ' << LINES[pos_start.line].line << '\n';

        int i       = 0;
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
             << name << " - " << details << '\n';
    }
};

string double_to_string(double n) {
    string output;

    {
        ostringstream oss;
        oss << fixed << n;
        output = oss.str();
    }

    int dot_pos = output.find_first_of('.');

    if (dot_pos != string::npos) {
        int pos = output.size() - 1;

        while (output[pos] == '0' && pos >= dot_pos)
            pos--;

        output.erase(output.begin() + pos + (pos != dot_pos), output.end());
    }

    if (output == "-0")
        return "0";

    return output;
}

struct IN_stream {
    virtual bool operator>>(string &str) = 0;
};

struct CIN_stream : public IN_stream {
    bool operator>>(string &str) override {
        getline(cin, str);
        return true;
    }
};

IN_stream *cin_stream = new CIN_stream;

struct IN_file_stream : public IN_stream {
    ifstream *f = nullptr;
    string path;

    bool operator>>(string &str) override {
        if (getline(*f, str))
            return true;

        return false;
    }

    ~IN_file_stream() {
        f->close();
        delete f;
    }
};

struct OUT_file_stream {
    ofstream *g = nullptr;
    string path;
    string *mode;

    template <class T>
    void operator<<(const T &str) { (*g) << str; }

    ~OUT_file_stream() {
        g->close();
        delete g;
    }
};
