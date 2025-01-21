// Copyright 2025 solar-mist

#ifndef VIPER_FRAMEWORK_PARSER_AST_GLOBAL_NAMESPACE_H
#define VIPER_FRAMEWORK_PARSER_AST_GLOBAL_NAMESPACE_H 1

#include "parser/ast/ASTNode.h"

#include <memory>
#include <string>
#include <vector>

namespace parser
{
    class Namespace : public ASTNode
    {
    public:
        Namespace(bool exported, std::string name, std::vector<ASTNodePtr> body, ScopePtr scope, lexer::Token token);

        virtual vipir::Value* codegen(vipir::IRBuilder& builder, vipir::Module& module, diagnostic::Diagnostics& diag) override;

        virtual void semanticCheck(diagnostic::Diagnostics& diag, bool& exit, bool statement) override;

        virtual void typeCheck(diagnostic::Diagnostics& diag, bool& exit) override;
        virtual bool triviallyImplicitCast(diagnostic::Diagnostics& diag, Type* destType) override;

    private:
        std::string mName;
        std::vector<ASTNodePtr> mBody;
        ScopePtr mScope;
    };
    using NamespacePtr = std::unique_ptr<Namespace>;
}

#endif // VIPER_FRAMEWORK_PARSER_AST_GLOBAL_NAMESPACE_H