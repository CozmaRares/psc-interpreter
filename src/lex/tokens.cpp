#include "tokens.hpp"

std::ostream& operator<<(std::ostream& os, Token::Kind kind) {
    switch (kind) {
        // literals
        case Token::Kind::Number: os << "Number"; break;
        case Token::Kind::Char: os << "Char"; break;
        case Token::Kind::String: os << "String"; break;
        case Token::Kind::Identifier: os << "Identifier"; break;

        // constants
        case Token::Kind::Null: os << "Null"; break;
        case Token::Kind::True: os << "True"; break;
        case Token::Kind::False: os << "False"; break;

        // keywords
        case Token::Kind::Let: os << "Let"; break;
        case Token::Kind::If: os << "If"; break;
        case Token::Kind::Then: os << "Then"; break;
        case Token::Kind::Else: os << "Else"; break;
        case Token::Kind::End: os << "End"; break;
        case Token::Kind::For: os << "For"; break;
        case Token::Kind::Execute: os << "Execute"; break;
        case Token::Kind::While: os << "While"; break;
        case Token::Kind::Do: os << "Do"; break;
        case Token::Kind::Until: os << "Until"; break;
        case Token::Kind::Print: os << "Print"; break;
        case Token::Kind::Read: os << "Read"; break;
        case Token::Kind::Throw: os << "Throw"; break;
        case Token::Kind::Try: os << "Try"; break;
        case Token::Kind::Catch: os << "Catch"; break;
        case Token::Kind::Function: os << "Function"; break;
        case Token::Kind::Return: os << "Return"; break;
        case Token::Kind::Continue: os << "Continue"; break;
        case Token::Kind::Break: os << "Break"; break;
        case Token::Kind::Include: os << "Include"; break;
        case Token::Kind::Run: os << "Run"; break;

        // Token::operators
        case Token::Kind::Plus: os << "Plus"; break;
        case Token::Kind::Minus: os << "Minus"; break;
        case Token::Kind::Multiply: os << "Multiply"; break;
        case Token::Kind::Divide: os << "Divide"; break;
        case Token::Kind::Modulo: os << "Modulo"; break;
        case Token::Kind::Equals: os << "Equals"; break;
        case Token::Kind::Less: os << "Less"; break;
        case Token::Kind::LessEqual: os << "LessEqual"; break;
        case Token::Kind::Greater: os << "Greater"; break;
        case Token::Kind::GreaterEqual: os << "GreaterEqual"; break;
        case Token::Kind::Different: os << "Different"; break;
        case Token::Kind::Assignment: os << "Assignment"; break;
        case Token::Kind::Or: os << "Or"; break;
        case Token::Kind::And: os << "And"; break;

        // delimiters
        case Token::Kind::ParenLeft: os << "ParenLeft"; break;
        case Token::Kind::ParenRight: os << "ParenRight"; break;
        case Token::Kind::BracketLeft: os << "BracketLeft"; break;
        case Token::Kind::BracketRight: os << "BracketRight"; break;
        case Token::Kind::CurlyLeft: os << "CurlyLeft"; break;
        case Token::Kind::CurlyRight: os << "CurlyRight"; break;
        case Token::Kind::Comma: os << "Comma"; break;
        case Token::Kind::Colon: os << "Colon"; break;
        case Token::Kind::Endline: os << "Endline"; break;
        case Token::Kind::EOI: os << "EOI"; break;

        default: os << "Unknown token kind";
    }
    return os;
}
