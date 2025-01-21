// Copyright 2024 solar-mist

#include "parser/ast/expression/CallExpression.h"
#include "parser/ast/expression/VariableExpression.h"

#include "type/FunctionType.h"
#include "type/PointerType.h"

#include <vipir/Module.h>

#include <vipir/IR/Function.h>
#include <vipir/IR/Instruction/CallInst.h>
#include <vipir/IR/Instruction/LoadInst.h>

#include <algorithm>

namespace parser
{
    CallExpression::CallExpression(Scope* scope, ASTNodePtr callee, std::vector<ASTNodePtr> parameters)
        : ASTNode(scope, callee->getErrorToken())
        , mCallee(std::move(callee))
        , mParameters(std::move(parameters))
        , mFakeFunction({{},{}})
    {
    }

    vipir::Value* CallExpression::codegen(vipir::IRBuilder& builder, vipir::Module& module, diagnostic::Diagnostics& diag)
    {
        vipir::Value* callee;
        if (mBestViableFunction == &mFakeFunction)
        {
            callee = mCallee->codegen(builder, module, diag);
        }
        else
        {
            callee = mBestViableFunction->getLatestValue();
        }

        std::vector<vipir::Value*> parameters;
        for (auto& parameter : mParameters)
        {
            parameters.push_back(parameter->codegen(builder, module, diag));
        }

        return builder.CreateCall(static_cast<vipir::Function*>(callee), std::move(parameters));
    }

    void CallExpression::semanticCheck(diagnostic::Diagnostics& diag, bool& exit, bool statement)
    {
        mCallee->semanticCheck(diag, exit, false);
        for (auto& parameter : mParameters)
        {
            parameter->semanticCheck(diag, exit, false);
        }
        if (statement)
        {
            if (mBestViableFunction->pure)
            {
                diag.compilerWarning(
                    "unused",
                    mErrorToken.getStartLocation(),
                    mErrorToken.getEndLocation(),
                    std::format("statement has no effect")
                );
            }
        }
    }

    void CallExpression::typeCheck(diagnostic::Diagnostics& diag, bool& exit)
    {
        mCallee->typeCheck(diag, exit);
        for (auto& parameter : mParameters)
        {
            parameter->typeCheck(diag, exit);
        }
        mBestViableFunction = getBestViableFunction(diag);

        if (!mBestViableFunction)
        {
            exit = true; // Error will have been reported in getBestViableFunction
            mType = Type::Get("error-type");
        }
        else
        {
            auto functionType = static_cast<FunctionType*>(mBestViableFunction->type);
            mType = functionType->getReturnType();
            unsigned int index = 0;
            for (auto& parameter : mParameters)
            {
                auto argumentType = functionType->getArgumentTypes()[index];
                if (parameter->getType() != argumentType)
                {
                    if (parameter->implicitCast(diag, argumentType))
                    {
                        parameter = Cast(parameter, argumentType);
                    }
                    else
                    {
                        diag.reportCompilerError(
                            mErrorToken.getStartLocation(),
                            mErrorToken.getEndLocation(),
                            std::format("no matching function for call to '{}{}(){}'",
                                fmt::bold, mBestViableFunction->name, fmt::defaults)
                        );
                        exit = true;
                        mType = Type::Get("error-type");
                    }
                }
            }
        }
    }

    bool CallExpression::triviallyImplicitCast(diagnostic::Diagnostics&, Type*)
    {
        return false;
    }

    struct ViableFunction
    {
        Symbol* symbol;
        int score;
        bool disallowed;
    };

    Symbol* CallExpression::getBestViableFunction(diagnostic::Diagnostics& diag)
    {
        if (auto var = dynamic_cast<VariableExpression*>(mCallee.get()))
        {
            if (var->getType()->isPointerType())
            {
                auto pointerType = static_cast<PointerType*>(var->getType());
                if (!pointerType->getPointeeType()->isFunctionType())
                {
                    diag.reportCompilerError(
                        mErrorToken.getStartLocation(),
                        mErrorToken.getEndLocation(),
                        std::format("'{}{}{}' cannot be used as a function",
                            fmt::bold, var->getName(), fmt::defaults)
                    );
                    return nullptr;
                }
                mBestViableFunction = &mFakeFunction;
                mFakeFunction.type = pointerType->getPointeeType();
                mFakeFunction.name = var->getName();
                return mBestViableFunction;
            }

            auto candidateFunctions = mScope->getCandidateFunctions(var->getNames());
            
            // Find all viable functions
            for (auto it = candidateFunctions.begin(); it != candidateFunctions.end();)
            {
                auto candidate = *it;
                if (!candidate->type->isFunctionType()) it = candidateFunctions.erase(it);
                else
                {
                    auto functionType = static_cast<FunctionType*>(candidate->type);
                    auto arguments = functionType->getArgumentTypes();
                    if (arguments.size() != mParameters.size()) it = candidateFunctions.erase(it);
                    else ++it;
                }
            }

            std::vector<ViableFunction> viableFunctions;
            for (auto& candidate : candidateFunctions)
            {
                auto functionType = static_cast<FunctionType*>(candidate->type);
                int score = 0;
                bool disallowed = false;
                for (size_t i = 0; i < mParameters.size(); ++i)
                {
                    auto castLevel = mParameters[i]->getType()->castTo(functionType->getArgumentTypes()[i]);
                    int multiplier = 0;
                    if (mParameters[i]->getType() == functionType->getArgumentTypes()[i]) multiplier = 0;
                    else if (castLevel == Type::CastLevel::Implicit) multiplier = 1;
                    else if (castLevel == Type::CastLevel::ImplicitWarning) multiplier = 2;
                    else disallowed = true;
                    score += multiplier * (mParameters.size() - i); // Weight earlier scores more
                }
                if (!disallowed)
                {
                    viableFunctions.push_back({candidate, score});
                }
            }

            if (viableFunctions.empty())
            {
                diag.reportCompilerError(
                    mErrorToken.getStartLocation(),
                    mErrorToken.getEndLocation(),
                    std::format("no matching function for call to '{}{}(){}'",
                        fmt::bold, var->getName(), fmt::defaults)
                );
                return nullptr;
            }
            
            std::sort(viableFunctions.begin(), viableFunctions.end(), [](const auto& lhs, const auto& rhs){
                return lhs.score < rhs.score;
            });
            if (viableFunctions.size() >= 2)
            {
                if (viableFunctions[0].score == viableFunctions[1].score)
                {
                    diag.reportCompilerError(
                        mErrorToken.getStartLocation(),
                        mErrorToken.getEndLocation(),
                        std::format("call to '{}{}(){}' is ambiguous",
                            fmt::bold, var->getName(), fmt::defaults)
                    );
                    return nullptr;
                }
            }
            return viableFunctions.front().symbol;
        }
        return nullptr;
    }
}