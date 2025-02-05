#include "ast.hpp"

#include <algorithm>
#include <iostream>

namespace AST {
    void Number::visit() {
        std::cout << "Number: " << m_number << std::endl;
    }

    void Char::visit() {
        std::cout << "Char: " << m_chr << std::endl;
    }

    void String::visit() {
        std::cout << "String: " << m_string << std::endl;
    }

    void Array::visit() {
        std::cout << "Array: " << m_array.size() << std::endl;
    }

    void Dictionary::visit() {
        std::cout << "Dictionary: " << m_dictionary.size() << std::endl;
    }

    void BinaryOperation::visit() {
        std::cout << "BinaryOperation" << std::endl;
    }

    void UnaryOperation::visit() {
        std::cout << "UnaryOperation" << std::endl;
    }

    void VariableAccess::visit() {
    }

    void IndexAccess::visit() {
    }

    void Assignment::visit() {
        std::cout << "MemoryWrite: " << m_identifier
                  << ", indexes: " << m_indexes.size() << std::endl;
    }

    std::unique_ptr<Expressions> Expressions::FromNode(std::unique_ptr<Node>&& node) {
        std::vector<std::unique_ptr<Node>> vec;
        vec.push_back(std::move(node));
        return std::make_unique<Expressions>(std::move(vec));
    }

    void Expressions::extend(std::unique_ptr<Expressions>&& expressions) {
        if (expressions->m_expressions.empty())
            return;

        std::transform(
            expressions->m_expressions.begin(),
            expressions->m_expressions.end(),
            std::back_inserter(m_expressions), [](auto& node) {
                return std::move(node);
            });
    }

    void Expressions::visit() {
        std::cout << "Expressions: " << m_expressions.size() << std::endl;
        for (auto& node : m_expressions)
            node->visit();
    }

    void IfExpression::visit() {
        std::cout << "IfExpression" << std::endl;
    }

    void ForExpression::visit() {
        std::cout << "ForExpression" << std::endl;
    }

    void WhileExpression::visit() {
        std::cout << "WhileExpression" << std::endl;
    }

    void DoUntilExpression::visit() {
        std::cout << "DoUntilExpression" << std::endl;
    }

    void ThrowExpression::visit() {
        std::cout << "ThrowExpression" << std::endl;
    }

    void TryCatchExpression::visit() {
        std::cout << "TryCatchExpression" << std::endl;
    }

    void FunctionDefinitionExpression::visit() {
        std::cout << "FunctionDefinitionExpression" << std::endl;
    }

    void FunctionCallExpression::visit() {
        std::cout << "FunctionCallExpression" << std::endl;
    }

    void ReturnExpression::visit() {
        std::cout << "ReturnExpression" << std::endl;
    }

    void PrintExpression::visit() {
        std::cout << "WriteStdOutExpression" << std::endl;
    }

    void ReadExpression::visit() {
        std::cout << "ReadStdInExpression" << std::endl;
    }

    void ContinueExpression::visit() {
        std::cout << "ContinueExpression" << std::endl;
    }

    void BreakExpression::visit() {
        std::cout << "BreakExpression" << std::endl;
    }

    void IncludeExpression::visit() {
        std::cout << "IncludeExpression" << std::endl;
    }

    void RunExpression::visit() {
        std::cout << "RunExpression" << std::endl;
    }
}  // namespace AST
