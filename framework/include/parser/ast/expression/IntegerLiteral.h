// Copyright 2024 solar-mist

#ifndef VIPER_FRAMEWORK_PARSER_AST_EXPRESSION_INTEGER_LITERAL_H
#define VIPER_FRAMEWORK_PARSER_AST_EXPRESSION_INTEGER_LITERAL_H 1

#include "parser/ast/ASTNode.h"

#include <cstdint>
#include <memory>

namespace parser
{
    class IntegerLiteral : public ASTNode
    {
    friend class ::ASTNodeIntrospector;
    public:
        IntegerLiteral(Scope* scope, std::uintmax_t value, lexer::Token token);

        virtual std::vector<ASTNode*> getContained() const override;
        virtual ASTNodePtr clone(Scope* in) override;

        virtual vipir::Value* codegen(vipir::IRBuilder& builder, vipir::Module& module, diagnostic::Diagnostics& diag) override;

        virtual void semanticCheck(diagnostic::Diagnostics& diag, bool& exit, bool statement) override;

        virtual void typeCheck(diagnostic::Diagnostics& diag, bool& exit) override;
        virtual bool triviallyImplicitCast(diagnostic::Diagnostics& diag, Type* destType) override;

    private:
        std::uintmax_t mValue;
    };
    using IntegerLiteralPtr = std::unique_ptr<IntegerLiteral>;
}

#endif // VIPER_FRAMEWORK_PARSER_AST_EXPRESSION_INTEGER_LITERAL_H