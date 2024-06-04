// Copyright 2024 solar-mist

#include "parser/ast/expression/VariableExpression.h"

#include "symbol/Identifier.h"

#include "type/PointerType.h"

#include <vipir/IR/Instruction/AllocaInst.h>
#include <vipir/IR/Instruction/GEPInst.h>
#include <vipir/IR/Instruction/LoadInst.h>
#include <vipir/IR/Instruction/PtrCastInst.h>

#include "vipir/Type/PointerType.h"


namespace parser
{
    VariableExpression::VariableExpression(std::string&& name, Type* type, lexing::Token token)
        : mName(std::move(name))
        , mToken(std::move(token))
    {
        mType = type;
        mPreferredDebugToken = mToken;
    }

    std::string VariableExpression::getName()
    {
        return mName;
    }

    void VariableExpression::typeCheck(Scope* scope, diagnostic::Diagnostics& diag)
    {
    }

    vipir::Value* VariableExpression::emit(vipir::IRBuilder& builder, vipir::Module& module, Scope* scope, diagnostic::Diagnostics& diag)
    {
        LocalSymbol* local = scope->findVariable(mName);

        std::vector<std::string> symbols = symbol::GetSymbol({mName}, scope->getNamespaces());

        if (local)
        {
            if (local->alloca->isConstant()) return local->alloca;

            return builder.CreateLoad(local->alloca);
        }
        else if (scope->findOwner() != nullptr && scope->findOwner()->hasField(mName))
        {
            StructType* structType = scope->findOwner();
            vipir::Value* self = builder.CreateLoad(scope->findVariable("this")->alloca);
            vipir::Value* gep = builder.CreateStructGEP(self, structType->getFieldOffset(mName));

            if (structType->getField(mName)->type->isPointerType())
            {
                if (static_cast<PointerType*>(structType->getField(mName)->type)->getBaseType() == structType)
                {
                    vipir::Type* type = vipir::PointerType::GetPointerType(vipir::PointerType::GetPointerType(structType->getVipirType()));
                    gep = builder.CreatePtrCast(gep, type);
                }
            }

            return builder.CreateLoad(gep);
        }
        else
        {
            for (auto& symbol : symbols)
            {
                if (GlobalFunctions.contains(symbol))
                {
                    return GlobalFunctions.at(symbol).function;
                }
                else if (GlobalVariables.contains(symbol))
                {
                    vipir::Value* value = GlobalVariables[symbol].global;
                    if (value->isConstant()) return value;

                    if (value->getType()->isPointerType()) return builder.CreateLoad(value); // TODO: Something better than this
                }
            }
        }
        diag.compilerError(mToken.getStart(), mToken.getEnd(), std::format("identifier '{}{}{}' undeclared",
            fmt::bold, mName, fmt::defaults));
    }
}
