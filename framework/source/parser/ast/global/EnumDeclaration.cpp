// Copyright 2025 solar-mist

#include "parser/ast/global/EnumDeclaration.h"

#include "type/EnumType.h"

#include <vipir/IR/Constant/ConstantInt.h>

namespace parser
{
    EnumDeclaration::EnumDeclaration(bool exported, bool pending, std::string name, std::vector<EnumField> fields, Type* base, ScopePtr ownScope, lexer::Token token)
        : ASTNode(ownScope->parent, std::move(token))
        , mName(std::move(name))
        , mFields(std::move(fields))
        , mOwnScope(std::move(ownScope))
    {
        auto namespaces = mScope->getNamespaces();
        namespaces.push_back(mName);
        auto mangled = StructType::MangleName(namespaces);

        if (pending)
        {
            mType = PendingStructType::Create(mErrorToken, mangled, base);
        }
        else
        {
            mType = EnumType::Create(mangled, base);
        }

        mSymbolId = mScope->symbols.emplace_back(mName, mType).id;
        mScope->symbols.back().exported = exported;

        for (auto& field : mFields)
        {
            field.symbolId = mOwnScope->symbols.emplace_back(field.name, mType).id;
        }
    }

    vipir::Value* EnumDeclaration::codegen(vipir::IRBuilder& builder, vipir::Module& module, diagnostic::Diagnostics& diag)
    {
        for (auto& field : mFields)
        {
            vipir::Value* constant = vipir::ConstantInt::Get(module, field.value, mType->getVipirType());

            auto symbol = mOwnScope->getSymbol(field.symbolId);
            symbol->values.push_back(std::make_pair(nullptr, constant));
        }

        return nullptr;
    }

    void EnumDeclaration::semanticCheck(diagnostic::Diagnostics&, bool&, bool)
    {
    }

    void EnumDeclaration::typeCheck(diagnostic::Diagnostics&, bool&)
    {

    }

    bool EnumDeclaration::triviallyImplicitCast(diagnostic::Diagnostics&, Type*)
    {
        return false;
    }
}