// Copyright 2025 solar-mist

#include "parser/ast/global/ClassDeclaration.h"

#include "type/StructType.h"

namespace parser
{
    ClassField::ClassField(Type* type, std::string name)
        : type(type)
        , name(std::move(name))
    {
    }
    
    
    ClassDeclaration::ClassDeclaration(bool exported, bool pending, std::string name, std::vector<ClassField> fields, Scope* scope, lexer::Token token)
        : ASTNode(scope, nullptr, token)
        , mName(std::move(name))
        , mFields(std::move(fields))
    {
        mSymbolId = mScope->symbols.emplace_back(mName, mType).id;
        mScope->getSymbol(mSymbolId)->exported = exported;

        std::vector<StructType::Field> structTypeFields;
        for (auto& field : mFields)
        {
            structTypeFields.push_back(StructType::Field{false, field.name, field.type});
        }
        
        if (pending)
        {
            if (auto type = Type::Get(mName))
            {
                auto pendingType = dynamic_cast<PendingStructType*>(type);
                // TODO: Make sure pendingType exists

                pendingType->setFields(std::move(structTypeFields));
            }
            else
            {
                PendingStructType::Create(mErrorToken, mName, std::move(structTypeFields));
            }
        }
        else
            StructType::Create(mName, std::move(structTypeFields));
    }

    vipir::Value* ClassDeclaration::codegen(vipir::IRBuilder& builder, vipir::Module& module, diagnostic::Diagnostics& diag)
    {
        // TODO: Method codegen
        return nullptr;
    }

    void ClassDeclaration::semanticCheck(diagnostic::Diagnostics& diag, bool& exit, bool statement)
    {
        // TODO: Call semanticCheck on methods
    }
    
    void ClassDeclaration::typeCheck(diagnostic::Diagnostics& diag, bool& exit)
    {
        // TODO: Call typeCheck on methods
    }

    bool ClassDeclaration::triviallyImplicitCast(diagnostic::Diagnostics&, Type*)
    {
        return false;
    }
}