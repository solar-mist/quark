// Copyright 2024 solar-mist

#include "parser/ast/expression/IntegerLiteral.h"

#include <vipir/IR/Constant/ConstantInt.h>

#include <cmath>

namespace parser
{
    IntegerLiteral::IntegerLiteral(Scope* scope, std::uintmax_t value, lexer::Token token)
        : ASTNode(scope, Type::Get("i32"), token)
        , mValue(value)
    {
    }

    vipir::Value* IntegerLiteral::codegen(vipir::IRBuilder& builder, vipir::Module& module, diagnostic::Diagnostics& diag)
    {
        return vipir::ConstantInt::Get(module, mValue, mType->getVipirType());
    }
    
    void IntegerLiteral::typeCheck(diagnostic::Diagnostics&, bool&)
    {
    }

    void IntegerLiteral::semanticCheck(diagnostic::Diagnostics& diag, bool& exit, bool statement)
    {
    }

    bool IntegerLiteral::triviallyImplicitCast(diagnostic::Diagnostics& diag, Type* destType)
    {
        if (destType->isIntegerType())
        {
            if (mValue >= std::pow(2, destType->getSize()))
            {
                diag.compilerWarning(
                    "implicit",
                    mErrorToken.getStartLocation(),
                    mErrorToken.getEndLocation(),
                    std::format("integer literal with value '{}{}{}' is being narrowed to '{}{}{}'",
                        fmt::bold, mValue, fmt::defaults,
                        fmt::bold, mValue % (int)std::pow(2, destType->getSize()), fmt::defaults)
                );
            }

            mType = destType;
            return true;
        }
        return false;
    }
}