// Copyright 2024 solar-mist

#include "parser/ast/global/Function.h"

#include "symbol/Mangle.h"

#include <vipir/IR/Function.h>
#include <vipir/IR/Instruction/AllocaInst.h>

namespace parser
{
    FunctionArgument::FunctionArgument(Type* type, std::string name)
        : type(type)
        , name(std::move(name))
    {
    }
    
    
    Function::Function(bool exported, bool pure, std::string name, FunctionType* type, std::vector<FunctionArgument> arguments, std::vector<ASTNodePtr> body, ScopePtr scope, lexer::Token token)
        : ASTNode(scope->parent, type, token)
        , mPure(pure)
        , mName(std::move(name))
        , mArguments(std::move(arguments))
        , mBody(std::move(body))
        , mOwnScope(std::move(scope))
    {
        mSymbolId = mScope->symbols.emplace_back(mName, mType).id;
        mScope->getSymbol(mSymbolId)->pure = mPure;
        mScope->getSymbol(mSymbolId)->exported = exported;
        for (auto& argument : mArguments)
        {
            mOwnScope->symbols.emplace_back(argument.name, argument.type);
        }
        mOwnScope->isPureScope = mPure;
    }

    vipir::Value* Function::codegen(vipir::IRBuilder& builder, vipir::Module& module, diagnostic::Diagnostics& diag)
    {
        auto mangledName = mangle::MangleFunction(mName, static_cast<FunctionType*>(mType));

        auto functionType = static_cast<vipir::FunctionType*>(mType->getVipirType());
        auto function = vipir::Function::Create(functionType, module, mangledName, mPure);

        mScope->getSymbol(mSymbolId)->values.push_back(std::make_pair(nullptr, function));

        if (mBody.empty())
        {
            return function;
        }

        auto entryBB = vipir::BasicBlock::Create("", function);
        builder.setInsertPoint(entryBB);

        unsigned int index = 0;
        for (auto& argument : mArguments)
        {
            auto arg = function->getArgument(index);

            mOwnScope->resolveSymbol(argument.name)->values.push_back(std::make_pair(entryBB, arg));
        }

        for (auto& node : mBody)
        {
            node->codegen(builder, module, diag);
        }

        return function;
    }

    void Function::semanticCheck(diagnostic::Diagnostics& diag, bool& exit, bool statement)
    {
        for (auto& value : mBody)
        {
            value->semanticCheck(diag, exit, true);
        }
    }
    
    void Function::typeCheck(diagnostic::Diagnostics& diag, bool& exit)
    {
        for (auto& node : mBody)
        {
            node->typeCheck(diag, exit);
        }
    }

    bool Function::triviallyImplicitCast(diagnostic::Diagnostics&, Type*)
    {
        return false;
    }
}