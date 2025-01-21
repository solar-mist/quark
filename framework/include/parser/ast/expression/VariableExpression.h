// Copyright 2024 solar-mist

#ifndef VIPER_FRAMEWORK_PARSER_AST_EXPRESSION_VARIABLE_EXPRESSION_H
#define VIPER_FRAMEWORK_PARSER_AST_EXPRESSION_VARIABLE_EXPRESSION_H 1

#include "parser/ast/ASTNode.h"

#include <cstdint>
#include <memory>

namespace parser
{
    class VariableExpression : public ASTNode
    {
    friend class ::ASTNodeIntrospector;
    public:
        VariableExpression(Scope* scope, std::string name, lexer::Token token);
        VariableExpression(Scope* scope, std::vector<std::string> names, lexer::Token token); // TODO: Maybe just pass a full start and end sourcelocation pair here

        virtual vipir::Value* codegen(vipir::IRBuilder& builder, vipir::Module& module, diagnostic::Diagnostics& diag) override;

        virtual void semanticCheck(diagnostic::Diagnostics& diag, bool& exit, bool statement) override;

        virtual void typeCheck(diagnostic::Diagnostics& diag, bool& exit) override;
        virtual bool triviallyImplicitCast(diagnostic::Diagnostics& diag, Type* destType) override;

        std::string getName();
        std::vector<std::string> getNames();
        bool isQualified();

    private:
        std::vector<std::string> mNames;

        std::string reconstructNames();
    };
    using VariableExpressionPtr = std::unique_ptr<VariableExpression>;
}

#endif // VIPER_FRAMEWORK_PARSER_AST_EXPRESSION_VARIABLE_EXPRESSION_H