// Copyright 2025 solar-mist

#include "parser/ast/global/Namespace.h"

namespace parser
{
    Namespace::Namespace(bool exported, std::string name, std::vector<ASTNodePtr> body, ScopePtr scope, lexer::Token token)
        : ASTNode(scope->parent, nullptr, token)
        , mName(std::move(name))
        , mBody(std::move(body))
        , mScope(std::move(scope))
    {
    }

    vipir::Value* Namespace::codegen(vipir::IRBuilder& builder, vipir::Module& module, diagnostic::Diagnostics& diag)
    {
        for (auto& node : mBody)
        {
            node->codegen(builder, module, diag);
        }
        return nullptr;
    }

    void Namespace::semanticCheck(diagnostic::Diagnostics& diag, bool& exit, bool statement)
    {
        for (auto& value : mBody)
        {
            value->semanticCheck(diag, exit, true);
        }
    }
    
    void Namespace::typeCheck(diagnostic::Diagnostics& diag, bool& exit)
    {
        for (auto& node : mBody)
        {
            node->typeCheck(diag, exit);
        }
    }

    bool Namespace::triviallyImplicitCast(diagnostic::Diagnostics&, Type*)
    {
        return false;
    }
}