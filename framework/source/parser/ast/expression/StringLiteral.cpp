// Copyright 2024 solar-mist

#include "parser/ast/expression/StringLiteral.h"

#include "type/PointerType.h"

#include <vipir/IR/GlobalString.h>

#include <vipir/IR/Instruction/AddrInst.h>

namespace parser
{
    StringLiteral::StringLiteral(Scope* scope, std::string value, lexer::Token token)
        : ASTNode(scope, PointerType::Get(Type::Get("i8")), token)
        , mValue(std::move(value))
    {
    }

    std::vector<ASTNode*> StringLiteral::getContained() const
    {
        return {};
    }

    ASTNodePtr StringLiteral::clone(Scope* in)
    {
        return std::make_unique<StringLiteral>(in, mValue, mErrorToken);
    }

    vipir::Value* StringLiteral::codegen(vipir::IRBuilder& builder, vipir::Module& module, diagnostic::Diagnostics& diag)
    {
        vipir::GlobalString* string = vipir::GlobalString::Create(module, std::move(mValue));

        return builder.CreateAddrOf(string);
    }

    void StringLiteral::semanticCheck(diagnostic::Diagnostics& diag, bool& exit, bool statement)
    {
    }
    
    void StringLiteral::typeCheck(diagnostic::Diagnostics&, bool&)
    {
    }

    bool StringLiteral::triviallyImplicitCast(diagnostic::Diagnostics&, Type*)
    {
        return false;
    }
}