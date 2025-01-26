// Copyright 2024 solar-mist

#include "parser/ast/expression/BooleanLiteral.h"

#include <vipir/IR/Constant/ConstantBool.h>
#include <vipir/IR/Constant/ConstantInt.h>

#include <cmath>

namespace parser
{
    BooleanLiteral::BooleanLiteral(Scope* scope, bool value, lexer::Token token)
        : ASTNode(scope, Type::Get("bool"), token)
        , mValue(value)
    {
    }

    std::vector<ASTNode*> BooleanLiteral::getContained() const
    {
        return {};
    }

    ASTNodePtr BooleanLiteral::clone(Scope* in)
    {
        return std::make_unique<BooleanLiteral>(in, mValue, mErrorToken);
    }

    vipir::Value* BooleanLiteral::codegen(vipir::IRBuilder& builder, vipir::Module& module, diagnostic::Diagnostics& diag)
    {
        if (mType->isBooleanType())
        {
            return builder.CreateConstantBool(mValue);
        }
        return vipir::ConstantInt::Get(module, mValue, mType->getVipirType());
    }

    void BooleanLiteral::semanticCheck(diagnostic::Diagnostics& diag, bool& exit, bool statement)
    {
    }
    
    void BooleanLiteral::typeCheck(diagnostic::Diagnostics&, bool&)
    {
    }

    bool BooleanLiteral::triviallyImplicitCast(diagnostic::Diagnostics& diag, Type* destType)
    {
        if (destType->isIntegerType())
        {
            mType = destType;
            return true;
        }
        return false;
    }
}