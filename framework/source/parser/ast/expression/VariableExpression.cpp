// Copyright 2024 solar-mist

#include "parser/ast/expression/VariableExpression.h"
#include "type/FunctionType.h"

#include <vipir/IR/Function.h>
#include <vipir/IR/Instruction/LoadInst.h>
#include <vipir/IR/Instruction/AllocaInst.h>
#include <vipir/IR/Instruction/GEPInst.h>

#include <cmath>

namespace parser
{
    VariableExpression::VariableExpression(Scope* scope, std::string name, lexer::Token token)
        : ASTNode(scope, token)
        , mNames({std::move(name)})
        , mIsImplicitThis(false)
    {
    }

    VariableExpression::VariableExpression(Scope* scope, std::vector<std::string> names, lexer::Token token)
        : ASTNode(scope, token)
        , mNames(std::move(names))
        , mIsImplicitThis(false)
    {
    }

    vipir::Value* VariableExpression::codegen(vipir::IRBuilder& builder, vipir::Module& module, diagnostic::Diagnostics& diag)
    {
        if (mIsImplicitThis)
        {
            vipir::Value* self = mScope->resolveSymbol("this")->getLatestValue();
            vipir::Value* gep = builder.CreateStructGEP(self, mScope->owner->getFieldOffset(mNames.back()));
            return builder.CreateLoad(gep);
        }

        Symbol* symbol;
        if (isQualified()) symbol = mScope->resolveSymbol(mNames);
        else symbol = mScope->resolveSymbol(mNames.back());

        if (symbol->type->isFunctionType()) return symbol->getLatestValue();
        
        auto latestValue = symbol->getLatestValue(builder.getInsertPoint());
        if (dynamic_cast<vipir::AllocaInst*>(latestValue)) return builder.CreateLoad(latestValue);
        return latestValue;
    }

    void VariableExpression::semanticCheck(diagnostic::Diagnostics& diag, bool& exit, bool statement)
    {
    }
    
    void VariableExpression::typeCheck(diagnostic::Diagnostics& diag, bool& exit)
    {
        if (mScope->owner)
        {
            auto structField = mScope->owner->getField(mNames.back());
            if (structField)
            {
                mType = structField->type;
                mIsImplicitThis = true;
                return;
            }
            else
            {
                auto structMethod = mScope->owner->getMethod(mNames.back());
                if (structMethod)
                {
                    auto functionType = static_cast<FunctionType*>(structMethod->type);
                    mType = functionType->getReturnType();
                    mIsImplicitThis = true;
                    return;
                }
            }
        }

        Symbol* symbol;
        if (isQualified()) symbol = mScope->resolveSymbol(mNames);
        else symbol = mScope->resolveSymbol(mNames.back());

        if (!symbol)
        {
            diag.reportCompilerError(
                mErrorToken.getStartLocation(),
                mErrorToken.getEndLocation(),
                std::format("undeclared identifier '{}{}{}'",
                    fmt::bold, reconstructNames(), fmt::defaults)
            );
            exit = true;
            mType = Type::Get("error-type");
        }
        else
        {
            mType = symbol->type;
        }
    }

    bool VariableExpression::triviallyImplicitCast(diagnostic::Diagnostics& diag, Type* destType)
    {
        return false;
    }

    std::string VariableExpression::getName()
    {
        return mNames.back();
    }

    std::vector<std::string> VariableExpression::getNames()
    {
        return mNames;
    }

    bool VariableExpression::isQualified()
    {
        return mNames.size() > 1;
    }

    bool VariableExpression::isImplicitMember()
    {
        return mIsImplicitThis;
    }


    std::string VariableExpression::reconstructNames()
    {
        std::string ret;
        for (auto it = mNames.begin(); it != mNames.end() - 1; ++it)
        {
            ret += (*it) + "::";
        }
        ret += mNames.back();
        return ret;
    }
}