#include "Parser.h"

bool run_and_print(string input, IN_stream* in);
bool read_map(string what, unordered_map<string, string>& MAP);
string setup();

int main() {
    string input = setup();

    if (input.size()) {
        cout << "\n\nCheck Resources/" << input << "s.txt\n\n";
        return 0;
    }

    do {
        if (!WRITE_INDENT) {
            cout << '\n';
            WRITE_INDENT = true;
        }

        cout << MAP_FILE_NAMES_TO_NO_LINES[PROGRAM_NAME] + 1 << " >>> ";
        (*cin_stream) >> input;
    } while (run_and_print(input, cin_stream));

    delete cin_stream;
    return 0;
}

Interpreter_result run(string input, IN_stream* in) {
    LINES.emplace_back(GLOBAL_CONTEXT.name, input);
    PARENT_CONTEXTS.clear();

    Lexer_result r1 = Lexer(true).make_tokens();

    if (r1.is_error()) {
        r1.print_error();
        return Interpreter_result(false);
    }

    Parse_result r2 = Parser(r1.tokens, in).parse();

    if (r2.is_error()) {
        r2.print_error();
        return Interpreter_result(false);
    }

    if (r2.node == nullptr)
        return Interpreter_result(false);

    return r2.node->visit(&GLOBAL_CONTEXT);
}

bool run_and_print(string input, IN_stream* in) {
    Interpreter_result res = run(input, in);

    if (res.is_error()) {
        if (res.error.details.find(exit_error) == 0) {
            if (!WRITE_INDENT)
                cout << '\n';

            cout << '\n'
                 << ERRORS["Program quit with code"] << res.error.details.substr(strlen(exit_error));

            cout << "\n\n";
            return false;
        }

        res.print_error();
        return true;
    }

    if (!res.print)
        return true;

    if (!WRITE_INDENT)
        cout << '\n';

    for (int i = to_string(MAP_FILE_NAMES_TO_NO_LINES[PROGRAM_NAME]).size(); i > 0; i--)
        cout << ' ';

    cout << "   < " << res.value->repr() << '\n';

    return true;
}

void initialize() {
    LINES.clear();
    PARENT_CONTEXTS.clear();
    INP_STREAMS_EXTRAS.clear();
    MAP_FILE_NAMES_TO_NO_LINES.clear();

    GLOBAL_CONTEXT = Context(PROGRAM_NAME);

    GLOBAL_CONTEXT.memory.set(BUILT_INS["TRUE"], make_shared<Number_value>(1, true));
    GLOBAL_CONTEXT.memory.set(BUILT_INS["FALSE"], make_shared<Number_value>(0, true));

    Value_shared_ptr null_ = NULL_->copy();
    null_->is_const        = true;

    GLOBAL_CONTEXT.memory.set(BUILT_INS["NULL"], null_);

    const map<string, vector<string>> predef_func = {
        { BUILT_INS["exit"], {} },
        { BUILT_INS["reset"], {} },
        { BUILT_INS["int"], { BUILT_INS["value"] } },
        { BUILT_INS["size"], { BUILT_INS["array"] } },
        { BUILT_INS["type"], { BUILT_INS["value"] } },
        { BUILT_INS["locals"], {} },
        { BUILT_INS["number"], { BUILT_INS["value"] } },
        { BUILT_INS["string"], { BUILT_INS["value"] } },
        { BUILT_INS["globals"], {} },
        { BUILT_INS["open_file"], { BUILT_INS["identifier"], BUILT_INS["path"], BUILT_INS["mode"] } },
        { BUILT_INS["close_file"], { BUILT_INS["identifier"] } },
        { BUILT_INS["global_assign"], { BUILT_INS["name"], BUILT_INS["value"] } },
        { BUILT_INS["get_dict_keys"], { BUILT_INS["dictionary"] } }
    };

    for (const auto& func : predef_func)
        GLOBAL_CONTEXT.memory.set(func.first, make_shared<Predefined_function_value>(func.first, func.second));
}

// returns true if an error occured
bool read_map(string what, unordered_map<string, string>& MAP) {
    string s = "Resources/" + what + "s.txt";
    ifstream f(s);

    if (!f.is_open()) {
        cout << "File '" << what << "s.txt' not found.";
        return true;
    }

    int line_number  = 1;
    bool error_found = false;
    string line;
    Lexer l(false);
    what[0] -= 32;

    while (getline(f, line)) {
        l.text             = line;
        l.current_position = Position();
        l.advance();
        Lexer_result r = l.make_tokens();
        // delete last token because parsing isn't needed
        r.tokens.pop_back();

        if (r.is_error()) {
            cout << "Invalid syntax on line " << line_number;
            error_found = true;
            break;
        }

        if (r.tokens.size() == 0) {
            line_number++;
            continue;
        }

        if (r.tokens.size() != 2) {
            cout << "Error on line " << line_number << ". Found " << r.tokens.size() << " element"
                 << (r.tokens.size() != 1 ? "s " : " ") << "when there should be 2.";
            error_found = true;
            break;
        }

        if (r.tokens[0] != T_STRING || r.tokens[1] != T_STRING) {
            cout << "Didn't find 2 strings on line " << line_number;
            error_found = true;
            break;
        }

        if (MAP.find(r.tokens[0]->value()) == MAP.end()) {
            cout << what << " on line " << line_number << " unknown";
            error_found = true;
            break;
        }

        if (MAP[r.tokens[0]->value()] != "") {
            cout << what << " on line " << line_number << " already defined";
            error_found = true;
            break;
        }

        if (r.tokens[1]->value().size())
            MAP[r.tokens[0]->value()] = r.tokens[1]->value();
        else
            MAP[r.tokens[0]->value()] = r.tokens[0]->value();

        line_number++;
    }

    if (error_found) {
        f.close();
        return true;
    }

    if (line_number <= MAP.size()) {
        vector<string> not_assinged;

        for (auto& e : MAP)
            if (e.second == "")
                not_assinged.push_back(e.first);

        what[0] += 32;
        cout << "Following " << what << "s not found:";

        for (auto& a : not_assinged) {
            cout << '\n';
            for (auto& b : a) {
                if (b == '\'' || b == '"')
                    cout << '\\';
                cout << b;
            }
        }

        return true;
    }

    return false;
}

string setup() {
    pair<string, unordered_map<string, string>*> maps[] = {
        { "error message", &ERRORS },
        { "keyword", &KEYWORDS },
        { "built-in", &BUILT_INS },
        { "value", &VALUE_NAMES_MAP }
    };

    for (auto& p : maps)
        if (read_map(p.first, *p.second))
            return p.first;

    initialize();

    return "";
}
