#pragma once

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "../lex/tokens.hpp"

namespace AST {
    // base
    class Node {
      public:
        virtual void visit() = 0;

        // TODO:
        // virtual void getAssignableValue() = 0;
        // make copies for primitives (number, char, string)
        // assign the pointer for everything else

        virtual ~Node() {}
    };

    // values
    class Number : public Node {
        double m_number;

      public:
        explicit Number(const double& number) : m_number(number) {}

        void visit() override;
    };

    class Char : public Node {
        char m_chr;

      public:
        explicit Char(const char& chr) : m_chr(chr) {}

        void visit() override;
    };

    class String : public Node {
        std::string m_string;

      public:
        explicit String(const std::string& string)
            : m_string(string) {}

        void visit() override;
    };

    class Array : public Node {
        using Value = std::unique_ptr<Node>;
        std::vector<Value> m_array;

      public:
        explicit Array(std::vector<Value>&& array)
            : m_array(std::move(array)) {}

        void visit() override;
    };

    class Dictionary : public Node {
        using Value = std::unique_ptr<Node>;
        std::vector<std::pair<Value, Value>> m_dictionary;

      public:
        explicit Dictionary(std::vector<std::pair<Value, Value>>&& dictionary)
            : m_dictionary(std::move(dictionary)) {}

        void visit() override;
    };

    // operations
    class BinaryOperation : public Node {
        std::unique_ptr<Node> m_left;
        Token::Kind m_operation;
        std::unique_ptr<Node> m_right;

      public:
        explicit BinaryOperation(
            std::unique_ptr<Node>&& left,
            const Token::Kind& operation,
            std::unique_ptr<Node>&& right)
            : m_left(std::move(left)),
              m_operation(operation),
              m_right(std::move(right)) {}

        void visit() override;
    };

    class UnaryOperation : public Node {
        Token::Kind m_operation;
        std::unique_ptr<Node> m_value;

      public:
        explicit UnaryOperation(
            const Token::Kind& operation,
            std::unique_ptr<Node>&& value)
            : m_operation(operation),
              m_value(std::move(value)) {}

        void visit() override;
    };

    class VariableAccess : public Node {
        std::string m_identifier;

      public:
        explicit VariableAccess(const std::string& identifier)
            : m_identifier(identifier) {}

        void visit() override;
    };

    class IndexAccess : public Node {
        std::unique_ptr<Node> m_array;
        std::unique_ptr<Node> m_index;

      public:
        explicit IndexAccess(
            std::unique_ptr<Node>&& array,
            std::unique_ptr<Node>&& index)
            : m_array(std::move(array)),
              m_index(std::move(index)) {}

        void visit() override;
    };

    class Assignment : public Node {
        std::string m_identifier;
        std::vector<std::unique_ptr<Node>> m_indexes;
        std::unique_ptr<Node> m_value;

      public:
        explicit Assignment(
            const std::string& identifier,
            std::vector<std::unique_ptr<Node>> indexes,
            std::unique_ptr<Node>&& value)
            : m_identifier(identifier),
              m_indexes(std::move(indexes)),
              m_value(std::move(value)) {}

        void visit() override;
    };

    // flow control
    class Expressions : public Node {
        std::vector<std::unique_ptr<Node>> m_expressions;

      public:
        explicit Expressions(std::vector<std::unique_ptr<Node>>&& expressions)
            : m_expressions(std::move(expressions)) {}

        static std::unique_ptr<Expressions> FromNode(std::unique_ptr<Node>&& node);

        void extend(std::unique_ptr<Expressions>&& expressions);

        void visit() override;
    };

    class IfExpression : public Node {
        std::unique_ptr<Node> m_conditionNode;
        std::unique_ptr<Expressions> m_trueBody;
        std::optional<std::unique_ptr<Expressions>> m_falseBody;

      public:
        explicit IfExpression(
            std::unique_ptr<Node>&& conditionNode,
            std::unique_ptr<Expressions>&& trueBody,
            std::optional<std::unique_ptr<Expressions>>&& falseBody)
            : m_conditionNode(std::move(conditionNode)),
              m_trueBody(std::move(trueBody)),
              m_falseBody(std::move(falseBody)) {}

        void visit() override;
    };

    class ForExpression : public Node {
        std::string m_variableName;
        std::unique_ptr<Node> m_startValue;
        std::unique_ptr<Node> m_stopValue;
        std::optional<std::unique_ptr<Node>> m_skipValue;
        std::unique_ptr<Expressions> m_body;

      public:
        explicit ForExpression(
            const std::string& variableName,
            std::unique_ptr<Node>&& startValue,
            std::unique_ptr<Node>&& stopValue,
            std::optional<std::unique_ptr<Node>>&& skipValue,
            std::unique_ptr<Expressions>&& body)
            : m_variableName(variableName),
              m_startValue(std::move(startValue)),
              m_stopValue(std::move(stopValue)),
              m_skipValue(std::move(skipValue)),
              m_body(std::move(body)) {}

        void visit() override;
    };

    class WhileExpression : public Node {
        std::unique_ptr<Node> m_conditionNode;
        std::unique_ptr<Expressions> m_body;

      public:
        explicit WhileExpression(
            std::unique_ptr<Node>&& conditionNode,
            std::unique_ptr<Expressions>&& body)
            : m_conditionNode(std::move(conditionNode)),
              m_body(std::move(body)) {}

        void visit() override;
    };

    class DoUntilExpression : public Node {
        std::unique_ptr<Expressions> m_body;
        std::unique_ptr<Node> m_conditionNode;

      public:
        explicit DoUntilExpression(
            std::unique_ptr<Expressions>&& body,
            std::unique_ptr<Node>&& conditionNode)
            : m_body(std::move(body)),
              m_conditionNode(std::move(conditionNode)) {}

        void visit() override;
    };

    class ContinueExpression : public Node {
      public:
        ContinueExpression() {}

        void visit() override;
    };

    class BreakExpression : public Node {
      public:
        BreakExpression() {}

        void visit() override;
    };

    class TryCatchExpression : public Node {
        std::unique_ptr<Expressions> m_tryBody;
        std::string m_errorIdentifier;
        std::unique_ptr<Expressions> m_catchBody;

      public:
        explicit TryCatchExpression(
            std::unique_ptr<Expressions>&& tryBody,
            const std::string& errorIdentifier,
            std::unique_ptr<Expressions>&& catchBody)
            : m_tryBody(std::move(tryBody)),
              m_errorIdentifier(errorIdentifier),
              m_catchBody(std::move(catchBody)) {}

        void visit() override;
    };

    class ThrowExpression : public Node {
        std::unique_ptr<Node> m_value;

      public:
        explicit ThrowExpression(std::unique_ptr<Node>&& value) : m_value(std::move(value)) {}

        void visit() override;
    };

    // functions
    class FunctionDefinitionExpression : public Node {
        std::string m_identifier;
        std::vector<std::string> m_args;
        std::unique_ptr<Expressions> m_body;

      public:
        explicit FunctionDefinitionExpression(
            const std::string& identifier,
            const std::vector<std::string>& args,
            std::unique_ptr<Expressions>&& body)
            : m_identifier(identifier),
              m_args(args),
              m_body(std::move(body)) {}

        void visit() override;
    };

    class FunctionCallExpression : public Node {
        std::unique_ptr<Node> m_function;
        std::vector<std::unique_ptr<Node>> m_args;

      public:
        explicit FunctionCallExpression(
            std::unique_ptr<Node>&& function,
            std::vector<std::unique_ptr<Node>>&& args)
            : m_function(std::move(function)),
              m_args(std::move(args)) {}

        void visit() override;
    };

    class ReturnExpression : public Node {
        std::unique_ptr<Node> m_node;

      public:
        explicit ReturnExpression(std::unique_ptr<Node>&& node) : m_node(std::move(node)) {}

        void visit() override;
    };

    // I/O
    class PrintExpression : public Node {
        std::optional<std::string> m_fileIdentifier;
        std::vector<std::unique_ptr<Node>> m_nodes;

      public:
        explicit PrintExpression(
            const std::optional<std::string>& fileIdentifier,
            std::vector<std::unique_ptr<Node>>&& nodes)
            : m_fileIdentifier(std::move(fileIdentifier)),
              m_nodes(std::move(nodes)) {}

        void visit() override;
    };

    class ReadExpression : public Node {
        std::optional<std::string> m_fileIdentifier;
        std::vector<std::string> m_identifiers;

      public:
        explicit ReadExpression(
            const std::optional<std::string>& fileIdentifier,
            const std::vector<
                std::string>& identifiers)
            : m_fileIdentifier(fileIdentifier),
              m_identifiers(identifiers) {}

        void visit() override;
    };

    // run external program
    class IncludeExpression : public Node {
        std::string m_path;

      public:
        explicit IncludeExpression(const std::string& path) : m_path(path) {}

        void visit() override;
    };

    class RunExpression : public Node {
        std::string m_path;

      public:
        explicit RunExpression(const std::string& path) : m_path(path) {}

        void visit() override;
    };

}  // namespace AST
