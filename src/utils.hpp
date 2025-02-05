#pragma once

#include <optional>
#include <stdexcept>
#include <string>

std::string format_double(double value, int precision = 4);

class Error {
  public:
    enum class Kind {
        LexError,
        ParseError,
        RuntimeError,
    };

  private:
    const Kind m_kind;
    const std::string m_reason;
    const std::string m_details;

  public:
    Error(
        const Kind& kind,
        const std::string& reason,
        const std::string& details)
        : m_kind(kind),
          m_reason(reason),
          m_details(details) {}

    void print() const;
};

template <typename Value>
class Result {
    std::optional<Error> m_error;
    std::optional<Value> m_value;

    explicit Result(Error error)
        : m_error(std::move(error)), m_value(std::nullopt) {}

    explicit Result(Value value)
        : m_error(std::nullopt), m_value(std::move(value)) {}

  public:
    static Result Err(Error error) { return Result(std::move(error)); }

    static Result Ok(Value value) { return Result(std::move(value)); }

    bool isError() const { return m_error.has_value(); }

    Error getError() {
        return std::move(*m_error);
    }

    Value getValue() {
        return std::move(*m_value);
    }
};
