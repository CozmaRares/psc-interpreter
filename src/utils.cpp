#include "utils.hpp"

#include <iomanip>
#include <iostream>
#include <sstream>

std::string format_double(const double& value, const int& precision) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << value;

    std::string output = oss.str();

    size_t dot_pos = output.find('.');
    if (dot_pos != std::string::npos) {
        output.erase(output.find_last_not_of('0') + 1);
        if (output.back() == '.')
            output.pop_back();
    }

    if (output == "-0")
        output = "0";

    return output;
}

std::ostream& operator<<(std::ostream& os, const Error::Kind& kind) {
    switch (kind) {
        case Error::Kind::LexError:
            os << "LexError";
            break;

        case Error::Kind::ParseError:
            os << "ParseError";
            break;

        case Error::Kind::RuntimeError:
            os << "RuntimeError";
            break;

        default: os << "Unknown error kind";
    }
    return os;
}

void Error::print() const {
    std::cout
        << "Kind:" << m_kind
        << "\nReason: " << m_reason
        << "\nDetails: " << m_details
        << std::endl;
}
