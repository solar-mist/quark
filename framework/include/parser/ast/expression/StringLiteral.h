// Copyright 2024 solar-mist

#ifndef VIPER_FRAMEWORK_PARSER_AST_EXPRESSION_STRING_LITERAL_H
#define VIPER_FRAMEWORK_PARSER_AST_EXPRESSION_STRING_LITERAL_H 1

#include "parser/ast/ASTNode.h"

namespace parser
{
    class StringLiteral : public ASTNode
    {
    public:
        StringLiteral(Scope* scope, std::string value, lexer::Token token);

        virtual std::vector<ASTNode*> getContained() const override;
        virtual ASTNodePtr clone(Scope* in) override;

        virtual vipir::Value* codegen(vipir::IRBuilder& builder, vipir::Module& module, diagnostic::Diagnostics& diag) override;

        virtual void semanticCheck(diagnostic::Diagnostics& diag, bool& exit, bool statement) override;

        virtual void typeCheck(diagnostic::Diagnostics& diag, bool& exit) override;
        virtual bool triviallyImplicitCast(diagnostic::Diagnostics& diag, Type* destType) override;

    private:
        std::string mValue;
    };
    using StringLiteralPtr = std::unique_ptr<StringLiteral>;
}

#endif // VIPER_FRAMEWORK_PARSER_AST_EXPRESSION_STRING_LITERAL_H