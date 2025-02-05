#include <iostream>

#include "lex/lexer.hpp"
#include "parse/parser.hpp"
#include "utils.hpp"

#define ASSERT_NO_ERROR(result)    \
    if (result.isError()) {        \
        result.getError().print(); \
        return 1;                  \
    }

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
    std::string input = "let a <- 1";

    std::cout << input << std::endl;

    auto lexResult = Lexer::Tokenize(input);
    ASSERT_NO_ERROR(lexResult);

    auto tokens = lexResult.getValue();

    auto parseResult = Parser::Parse(std::move(tokens));
    ASSERT_NO_ERROR(parseResult);

    parseResult.getValue()->visit();
}
