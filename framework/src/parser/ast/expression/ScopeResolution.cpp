// Copyright 2024 solar-mist


#include "parser/ast/expression/ScopeResolution.h"

#include <iostream>

#include "parser/ast/expression/VariableExpression.h"

#include "symbol/Identifier.h"

#include <vipir/IR/Instruction/LoadInst.h>

namespace parser
{
    ScopeResolution::ScopeResolution(ASTNodePtr left, lexing::Token token, ASTNodePtr right)
        : mLeft(std::move(left))
        , mToken(std::move(token))
        , mRight(std::move(right))
    {
        std::vector<std::string> symbols = symbol::GetSymbol(getNames(), getNames());
        
        for (auto symbol : symbols)
        {
            if (GlobalVariables.contains(symbol))
            {
                mType = GlobalVariables[symbol].type;
            }
            else if (GlobalFunctions.contains(symbol))
            {
                mType = GlobalFunctions[symbol].type;
            }
        }
        mPreferredDebugToken = token;
    }

    void ScopeResolution::typeCheck(Scope* scope, diagnostic::Diagnostics& diag)
    {
        mLeft->typeCheck(scope, diag);
        mRight->typeCheck(scope, diag);
    }

    std::vector<std::string> ScopeResolution::getNames()
    {
        std::vector<std::string> ret;
        
        if (auto var = dynamic_cast<VariableExpression*>(mLeft.get()))
        {
            ret.push_back(var->getName());
        }
        if (auto var = dynamic_cast<VariableExpression*>(mRight.get()))
        {
            ret.push_back(var->getName());
        }
        else if (auto scope = dynamic_cast<ScopeResolution*>(mRight.get()))
        {
            auto names = scope->getNames();
            std::move(names.begin(), names.end(), std::back_inserter(ret));
        }

        return ret;
    }

    vipir::Value* ScopeResolution::emit(vipir::IRBuilder& builder, vipir::Module& module, Scope* scope, diagnostic::Diagnostics& diag)
    {
        std::vector<std::string> symbols = symbol::GetSymbol(getNames(), scope->getNamespaces());
        
        for (auto symbol : symbols)
        {
            if (GlobalVariables.find(symbol) != GlobalVariables.end())
            {
                vipir::Value* value = GlobalVariables[symbol].global;
                if (value->isConstant()) return value;

                if (value->getType()->isPointerType()) return builder.CreateLoad(value); // TODO: Something better than this
            }
        }

        diag.compilerError(mToken.getStart(), mToken.getEnd(), std::format("unknown identifier '{}{}{}'", fmt::bold, getNames().back(), fmt::defaults));

        return nullptr;
    }
}