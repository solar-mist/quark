// Copyright 2024 solar-mist

#ifndef VIPER_FRAMEWORK_PARSER_AST_EXPRESSION_UNARY_EXPRESSION_H
#define VIPER_FRAMEWORK_PARSER_AST_EXPRESSION_UNARY_EXPRESSION_H 1

#include "parser/ast/ASTNode.h"

#include <memory>

namespace parser
{
    class UnaryExpression : public ASTNode
    {
    friend class ::ASTNodeIntrospector;
    public:
        enum class Operator 
        {
            // Mathematical
            Negate,

            // Special
            AddressOf,
            Indirection
        };

        UnaryExpression(Scope* scope, ASTNodePtr operand, lexer::TokenType operatorToken, bool postfix, lexer::Token token);
        UnaryExpression(Scope* scope, ASTNodePtr operand, Operator op, bool postfix, lexer::Token token);

        virtual std::vector<ASTNode*> getContained() const override;
        virtual ASTNodePtr clone(Scope* in) override;

        virtual vipir::Value* codegen(vipir::IRBuilder& builder, vipir::Module& module, diagnostic::Diagnostics& diag) override;

        virtual void semanticCheck(diagnostic::Diagnostics& diag, bool& exit, bool statement) override;

        virtual void typeCheck(diagnostic::Diagnostics& diag, bool& exit) override;
        virtual bool triviallyImplicitCast(diagnostic::Diagnostics& diag, Type* destType) override;

    private:
        ASTNodePtr mOperand;
        Operator mOperator;
        bool mPostfix;
    };
    using UnaryExpressionPtr = std::unique_ptr<UnaryExpression>;
}

#endif // VIPER_FRAMEWORK_PARSER_AST_EXPRESSION_UNARY_EXPRESSION_H
