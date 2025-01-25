// Copyright 2024 solar-mist

#include "parser/ast/expression/CastExpression.h"

#include <vipir/IR/Instruction/TruncInst.h>
#include <vipir/IR/Instruction/SExtInst.h>
#include <vipir/IR/Instruction/BinaryInst.h>
#include <vipir/IR/Constant/ConstantInt.h>

#include <cmath>

namespace parser
{
    CastExpression::CastExpression(Scope* scope, ASTNodePtr value, Type* destType)
        : ASTNode(scope, destType, lexer::Token("", lexer::TokenType::Error, {}, {}))
        , mValue(std::move(value))
    {
    }

    vipir::Value* CastExpression::codegen(vipir::IRBuilder& builder, vipir::Module& module, diagnostic::Diagnostics& diag)
    {
        auto value = mValue->codegen(builder, module, diag);
        if (mType->isIntegerType() && mValue->getType()->isIntegerType())
        {
            if (mType->getSize() < mValue->getType()->getSize())
            {
                return builder.CreateTrunc(value, mType->getVipirType());
            }
            else
            {
                return builder.CreateSExt(value, mType->getVipirType());
            }
        }
        if (mType->isBooleanType() && mValue->getType()->isIntegerType())
        {
            auto constantInt = vipir::ConstantInt::Get(module, 0, mValue->getType()->getVipirType());
            return builder.CreateCmpNE(value, constantInt);
        }
        if (mType->isIntegerType() && mValue->getType()->isEnumType())
        {
            if (mType->getSize() == mValue->getType()->getSize())
            {
                return value;
            }
            if (mType->getSize() < mValue->getType()->getSize())
            {
                return builder.CreateTrunc(value, mType->getVipirType());
            }
            else
            {
                return builder.CreateSExt(value, mType->getVipirType());
            }
        }
        return nullptr; // Should be unreachable
    }

    void CastExpression::semanticCheck(diagnostic::Diagnostics& diag, bool& exit, bool statement)
    {
        mValue->semanticCheck(diag, exit, false);
    }
    
    void CastExpression::typeCheck(diagnostic::Diagnostics& diag, bool& exit)
    {
        mValue->typeCheck(diag, exit);
    }

    bool CastExpression::triviallyImplicitCast(diagnostic::Diagnostics& diag, Type* destType)
    {
        return mValue->triviallyImplicitCast(diag, destType);
    }
}