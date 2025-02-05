#include "lexer.hpp"

#include <sstream>
#include <string>
#include <unordered_map>

#include "tokens.hpp"

template <typename T>
Result<T> Lexer::Ok(T value) {
    return Result<T>::Ok(std::move(value));
}

template <typename T>
Result<T> Lexer::Err(const std::string& reason) {
    std::string details = getErrorDetails();
    return Result<T>::Err(Error(Error::Kind::LexError, reason, details));
}

template <typename T>
Result<T> Lexer::Err(Error error) {
    return Result<T>::Err(error);
}

std::string Lexer::getErrorDetails() {
    std::ostringstream oss;
    oss << "character << " << m_currentChar;
    oss << '(' << static_cast<int>(m_currentChar) << ')';
    oss << " >> at index " << m_currentPosition;

    auto left  = m_currentPosition < 10 ? 0 : m_currentPosition - 10;
    auto count = m_currentPosition < 10 ? m_currentPosition : 10;
    oss << " << \"" << m_text.substr(left, count) << "\"";

    return oss.str();
}

// clang-format off
const std::unordered_map<std::string, Token::Kind> KEYWORDS = {
    { "null",     Token::Kind::Null     },
    { "true",     Token::Kind::True     },
    { "false",    Token::Kind::False    },
    { "let",      Token::Kind::Let      },
    { "if",       Token::Kind::If       },
    { "then",     Token::Kind::Then     },
    { "else",     Token::Kind::Else     },
    { "end",      Token::Kind::End      },
    { "for",      Token::Kind::For      },
    { "execute",  Token::Kind::Execute  },
    { "while",    Token::Kind::While    },
    { "do",       Token::Kind::Do       },
    { "until",    Token::Kind::Until    },
    { "print",    Token::Kind::Print    },
    { "read",     Token::Kind::Read     },
    { "throw",    Token::Kind::Throw    },
    { "try",      Token::Kind::Try      },
    { "catch",    Token::Kind::Catch    },
    { "function", Token::Kind::Function },
    { "return",   Token::Kind::Return   },
    { "continue", Token::Kind::Continue },
    { "break",    Token::Kind::Break    },
    { "include",  Token::Kind::Include  },
    { "run",      Token::Kind::Run      },
    { "or",       Token::Kind::Or       },
    { "and",      Token::Kind::And      },
};
// clang-format on

const char COMMENT_CHAR = '$';

bool isDigit(const char& c) { return c >= '0' && c <= '9'; }

bool isIdentifierChar(const char& c, const bool& first = false) {
    bool lower = c >= 'a' && c <= 'z';
    bool upper = c >= 'A' && c <= 'Z';
    bool digit = isDigit(c) && !first;

    return c == '_' || lower || upper || digit;
}

void Lexer::advance() {
    m_currentPosition++;

    m_currentChar =
        m_currentPosition < m_text.size()
            ? m_text[m_currentPosition]
            : '\0';
}

Token* Lexer::makeNumber() {
    std::ostringstream number;
    bool isEmpty = true;

    while (isDigit(m_currentChar)) {
        number << m_currentChar;
        isEmpty = false;
        advance();
    }

    if (m_currentChar != '.')
        goto __make_number_reutrn;

    if (isEmpty)
        number << '0';

    number << '.';
    advance();

    while (isDigit(m_currentChar)) {
        number << m_currentChar;
        advance();
    }

__make_number_reutrn:
    auto str     = number.str();
    Token* token = new Token(Token::Kind::Number, str);
    return token;
}

char getEscapedChar(const char& c) {
    switch (c) {
        case '0': return '\0';
        case 'a': return '\a';
        case 'b': return '\b';
        case 'f': return '\f';
        case 'n': return '\n';
        case 'r': return '\r';
        case 't': return '\t';
        case 'v': return '\v';
        case '"': return '"';
        case '\'': return '\'';
        default: return c;
    }
}

Result<Token*> Lexer::makeChar() {
    advance();  // currentPosition already on '

    char ch;

    if (m_currentChar == '\\') {
        advance();
        ch = getEscapedChar(m_currentChar);
    } else
        ch = m_currentChar;

    advance();

    if (m_currentChar != '\'') {
        std::string details = m_currentChar == '\0' ? "Expected <'>(apostrophe)" : "Use <\"...\"> for strings";
        return Err<Token*>(details);
    }

    advance();

    return Ok(new Token(Token::Kind::Char, std::string(1, ch)));
}

Result<Token*> Lexer::makeString() {
    advance();  // currentPosition already on "

    std::ostringstream str;
    bool isEscaped = false;

    while ((m_currentChar != '"' || isEscaped) && m_currentChar != '\0') {
        if (isEscaped) {
            str << getEscapedChar(m_currentChar);
            isEscaped = false;
        } else if (m_currentChar == '\\')
            isEscaped = true;
        else
            str << m_currentChar;
        advance();
    }

    if (m_currentChar != '"')
        return Err<Token*>("Expected <\">(double quote)");

    advance();

    return Ok(new Token(Token::Kind::String, str.str()));
}

Token* Lexer::makeFromLiteral() {
    std::ostringstream ident;

    ident << m_currentChar;
    advance();  // currentPosition already on an identifier's first character

    while (isIdentifierChar(m_currentChar)) {
        ident << m_currentChar;
        advance();
    }

    std::string id = ident.str();

    if (KEYWORDS.find(id) != KEYWORDS.end()) {
        Token* token = new Token(KEYWORDS.at(id), id);
        return token;
    }

    Token* token = new Token(Token::Kind::Identifier, id);
    return token;
}

Token* Lexer::makeOperator() {
    switch (m_currentChar) {
        case '+': return new Token(Token::Kind::Plus, "+");
        case '-': return new Token(Token::Kind::Minus, "-");
        case '*': return new Token(Token::Kind::Multiply, "*");
        case '/': return new Token(Token::Kind::Divide, "/");
        case '%': return new Token(Token::Kind::Modulo, "%");
        case '=': return new Token(Token::Kind::Equals, "=");
        default: break;
    }

    if (m_currentChar == '<') {
        std::ostringstream op;
        op << m_currentChar;
        Token::Kind kind = Token::Kind::Less;

        advance();
        bool match = true;

        switch (m_currentChar) {
            case '=': kind = Token::Kind::LessEqual; break;
            case '>': kind = Token::Kind::Different; break;
            case '-': kind = Token::Kind::Assignment; break;
            default: match = false; break;
        }

        if (match) {
            op << m_currentChar;
            advance();
        }

        return new Token(kind, op.str());
    }

    if (m_currentChar == '>') {
        std::ostringstream op;
        op << m_currentChar;
        Token::Kind kind = Token::Kind::Greater;

        advance();
        bool match = true;

        switch (m_currentChar) {
            case '=': kind = Token::Kind::GreaterEqual; break;
            default: match = false; break;
        }

        if (match) {
            op << m_currentChar;
            advance();
        }

        return new Token(kind, op.str());
    }

    return nullptr;
}

Token* Lexer::makeDelimiter() {
    switch (m_currentChar) {
        case '(': return new Token(Token::Kind::ParenLeft, "(");
        case ')': return new Token(Token::Kind::ParenRight, ")");
        case '[': return new Token(Token::Kind::BracketLeft, "[");
        case ']': return new Token(Token::Kind::BracketRight, "]");
        case '{': return new Token(Token::Kind::CurlyLeft, "{");
        case '}': return new Token(Token::Kind::CurlyRight, "}");
        case ',': return new Token(Token::Kind::Comma, ",");
        case ':': return new Token(Token::Kind::Colon, ":");
        case '\n': return new Token(Token::Kind::Endline, "\n");
        default: return nullptr;
    }
}

Result<Token*> Lexer::makeToken() {
    if (isDigit(m_currentChar))
        return Ok(makeNumber());

    if (m_currentChar == '\'')
        return makeChar();

    if (m_currentChar == '"')
        return makeString();

    if (isIdentifierChar(m_currentChar, true))
        return Ok(makeFromLiteral());

    Token* t;

    if ((t = makeOperator()) != nullptr ||
        (t = makeDelimiter()) != nullptr) {
        advance();
        return Ok(t);
    }

    return Err<Token*>("Unknown character");
}

Result<std::vector<std::unique_ptr<Token>>> Lexer::Tokenize(const std::string& text) {
    Lexer lexer(text);

    std::vector<std::unique_ptr<Token>> tokens;

    while (lexer.m_currentChar) {
        if (lexer.m_currentChar == COMMENT_CHAR)
            break;

        // skip whitespace
        if (lexer.m_currentChar == ' ' || lexer.m_currentChar == '\t' || lexer.m_currentChar == '\r') {
            lexer.advance();
            continue;
        }

        auto result = lexer.makeToken();

        if (result.isError())
            return Err<std::vector<std::unique_ptr<Token>>>(result.getError());

        Token* token = result.getValue();
        tokens.push_back(std::unique_ptr<Token>(token));
    }

    Token* token = new Token(Token::Kind::EOI, "");
    tokens.push_back(std::unique_ptr<Token>(token));
    return Ok(std::move(tokens));
}
