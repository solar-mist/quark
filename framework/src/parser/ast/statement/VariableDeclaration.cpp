// Copyright 2024 solar-mist

#include "parser/ast/statement/VariableDeclaration.h"

#include <iostream>
#include <vipir/IR/Instruction/AllocaInst.h>
#include <vipir/IR/Instruction/StoreInst.h>

namespace parser
{
    VariableDeclaration::VariableDeclaration(Type* type, std::string&& name, ASTNodePtr&& initialValue)
        : mName(std::move(name))
        , mInitialValue(std::move(initialValue))
    {
        mType = type;
    }

    void VariableDeclaration::typeCheck(Scope* scope, diagnostic::Diagnostics& diag)
    {
        if (mInitialValue)
        {
            if (mInitialValue->getType() != mType)
            {
                diag.compilerError(mInitialValue->getDebugToken().getStart(), mInitialValue->getDebugToken().getEnd(), std::format("Variable of type '{}{}{}' cannot be initialized with a value of type '{}{}{}'",
                    fmt::bold, mType->getName(), fmt::defaults,
                    fmt::bold, mInitialValue->getType()->getName(), fmt::defaults));
            }
            mInitialValue->typeCheck(scope, diag);
        }
    }

    vipir::Value* VariableDeclaration::emit(vipir::IRBuilder& builder, vipir::Module& module, Scope* scope, diagnostic::Diagnostics& diag)
    {
        vipir::AllocaInst* alloca = builder.CreateAlloca(mType->getVipirType());

        if (mInitialValue)
        {
            vipir::Value* initalValue = mInitialValue->emit(builder, module, scope, diag);
            builder.CreateStore(alloca, initalValue);
        }

        scope->locals[mName].alloca = alloca;

        return nullptr;
    }

}