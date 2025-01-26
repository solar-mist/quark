// Copyright 2024 solar-mist

#ifndef VIPER_FRAMEWORK_PARSER_AST_EXPRESSION_BOOLEAN_LITERAL_H
#define VIPER_FRAMEWORK_PARSER_AST_EXPRESSION_BOOLEAN_LITERAL_H 1

#include "parser/ast/ASTNode.h"

#include <cstdint>
#include <memory>

namespace parser
{
    class BooleanLiteral : public ASTNode
    {
    friend class ::ASTNodeIntrospector;
    public:
        BooleanLiteral(Scope* scope, bool value, lexer::Token token);

        virtual std::vector<ASTNode*> getContained() const override;
        virtual ASTNodePtr clone(Scope* in) override;

        virtual vipir::Value* codegen(vipir::IRBuilder& builder, vipir::Module& module, diagnostic::Diagnostics& diag) override;

        virtual void semanticCheck(diagnostic::Diagnostics& diag, bool& exit, bool statement) override;

        virtual void typeCheck(diagnostic::Diagnostics& diag, bool& exit) override;
        virtual bool triviallyImplicitCast(diagnostic::Diagnostics& diag, Type* destType) override;

    private:
        bool mValue;
    };
    using BooleanLiteralPtr = std::unique_ptr<BooleanLiteral>;
}

#endif // VIPER_FRAMEWORK_PARSER_AST_EXPRESSION_BOOLEAN_LITERAL_H