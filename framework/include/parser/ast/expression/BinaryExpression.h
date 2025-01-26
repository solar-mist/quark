// Copyright 2024 solar-mist

#ifndef VIPER_FRAMEWORK_PARSER_AST_EXPRESSION_BINARY_EXPRESSION_H
#define VIPER_FRAMEWORK_PARSER_AST_EXPRESSION_BINARY_EXPRESSION_H 1

#include "parser/ast/ASTNode.h"

#include <memory>

namespace parser
{
    class BinaryExpression : public ASTNode
    {
    friend class ::ASTNodeIntrospector;
    public:
        enum class Operator 
        {
            // Mathematical
            Add,
            Sub,
            Mul,
            Div,

            // Comparison
            Equal,
            NotEqual,
            LessThan,
            GreaterThan,
            LessEqual,
            GreaterEqual,

            // Assignment
            Assign,
        };

        BinaryExpression(Scope* scope, ASTNodePtr left, lexer::TokenType operatorToken, ASTNodePtr right, lexer::Token token);
        BinaryExpression(Scope* scope, ASTNodePtr left, Operator op, ASTNodePtr right, lexer::Token token);

        virtual std::vector<ASTNode*> getContained() const override;
        virtual ASTNodePtr clone(Scope* in) override;

        virtual vipir::Value* codegen(vipir::IRBuilder& builder, vipir::Module& module, diagnostic::Diagnostics& diag) override;

        virtual void semanticCheck(diagnostic::Diagnostics& diag, bool& exit, bool statement) override;

        virtual void typeCheck(diagnostic::Diagnostics& diag, bool& exit) override;
        virtual bool triviallyImplicitCast(diagnostic::Diagnostics& diag, Type* destType) override;

    private:
        ASTNodePtr mLeft;
        ASTNodePtr mRight;
        Operator mOperator;
    };
    using BinaryExpressionPtr = std::unique_ptr<BinaryExpression>;
}

#endif // VIPER_FRAMEWORK_PARSER_AST_EXPRESSION_BINARY_EXPRESSION_H
