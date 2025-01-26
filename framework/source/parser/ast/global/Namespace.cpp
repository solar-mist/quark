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

    std::vector<ASTNode*> Namespace::getContained() const
    {
        std::vector<ASTNode*> ret;
        for (auto& node : mBody)
        {
            ret.push_back(node.get());
        }
        return ret;
    }

    ASTNodePtr Namespace::clone(Scope* in)
    {
        auto scope = mScope->clone(in);
        
        std::vector<ASTNodePtr> bodyClone;
        bodyClone.reserve(mBody.size());
        for (auto& node : mBody)
        {
            bodyClone.push_back(node->clone(scope.get()));
        }
        return std::make_unique<Namespace>(false, mName, std::move(bodyClone), std::move(scope), mErrorToken);
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