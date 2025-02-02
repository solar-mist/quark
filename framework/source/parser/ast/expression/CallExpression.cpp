// Copyright 2024 solar-mist

#include "parser/ast/expression/CallExpression.h"
#include "parser/ast/expression/VariableExpression.h"
#include "parser/ast/expression/MemberAccess.h"

#include "type/FunctionType.h"
#include "type/PointerType.h"

#include <functional>
#include <vipir/Module.h>

#include <vipir/IR/Function.h>
#include <vipir/IR/Instruction/CallInst.h>
#include <vipir/IR/Instruction/LoadInst.h>
#include <vipir/IR/Instruction/GEPInst.h>
#include <vipir/IR/Instruction/AddrInst.h>

#include <algorithm>

namespace parser
{
    CallExpression::CallExpression(Scope* scope, ASTNodePtr callee, std::vector<ASTNodePtr> parameters)
        : ASTNode(scope, callee->getErrorToken())
        , mCallee(std::move(callee))
        , mParameters(std::move(parameters))
        , mFakeFunction({{},{},{}})
        , mIsMemberFunction(false)
    {
    }

    std::vector<ASTNode*> CallExpression::getContained() const
    {
        std::vector<ASTNode*> ret = {mCallee.get()};
        for (auto& param : mParameters)
        {
            ret.push_back(param.get());
        }
        return ret;
    }

    ASTNodePtr CallExpression::clone(Scope* in)
    {
        std::vector<ASTNodePtr> parameters;
        parameters.reserve(mParameters.size());
        for (auto& param : mParameters)
        {
            parameters.push_back(param->clone(in));
        }
        return std::make_unique<CallExpression>(in, mCallee->clone(in), std::move(parameters));
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
        if (mIsMemberFunction)
        {
            // this parameter
            if (auto var = dynamic_cast<VariableExpression*>(mCallee.get()))
            {
                parameters.push_back(mScope->resolveSymbol("this")->getLatestValue());
            }
            else
            {
                auto member = static_cast<MemberAccess*>(mCallee.get());
                auto value = member->mStruct->codegen(builder, module, diag);
                if (member->mStruct->getType()->isStructType())
                {
                    vipir::Value* self = vipir::getPointerOperand(value);

                    vipir::Instruction* instruction = static_cast<vipir::Instruction*>(value);
                    instruction->eraseFromParent();

                    if (dynamic_cast<vipir::GEPInst*>(self))
                    {
                        value = self;
                    }
                    else
                    {
                        value = builder.CreateAddrOf(self);
                    }
                    parameters.push_back(value);
                }
                else
                {
                    parameters.push_back(value);
                }
            }
        }
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
        mBestViableFunction = getBestViableFunction(diag, exit);

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

    Symbol* CallExpression::getBestViableFunction(diagnostic::Diagnostics& diag, bool& exit)
    {
        if (dynamic_cast<VariableExpression*>(mCallee.get()) || dynamic_cast<MemberAccess*>(mCallee.get()))
        {
            std::vector<Symbol*> candidateFunctions;
            std::string errorName;

            if (auto var = dynamic_cast<VariableExpression*>(mCallee.get()))
            {
                errorName = var->getName();
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
                if (var->isImplicitMember())
                {
                    auto scopeOwner = mScope->findOwner();
                    StructType* structType = nullptr;
                    
                    if ((structType = dynamic_cast<StructType*>(scopeOwner)));
                    else if (auto pending = dynamic_cast<PendingStructType*>(scopeOwner))
                        structType = pending->get();

                    auto names = structType->getNames();
                    names.push_back(var->getName());
                    candidateFunctions = mScope->getCandidateFunctions(names);

                    errorName = structType->getName();
                    errorName += "::" + var->getName();

                    mIsMemberFunction = true;
                }
                // Templated function call so we need to instantiate
                else if (var->mTemplateParameters.size() > 0)
                {
                    auto symbol = mScope->resolveSymbol(var->getNames());
                    if (!symbol) return nullptr; // Error will be reported in VariableExpression
                    auto it = std::find_if(symbol->templated->instantiations.begin(), symbol->templated->instantiations.end(), [&var](auto& inst){
                        return inst.parameters == var->mTemplateParameters;
                    });
                    if (it != symbol->templated->instantiations.end())
                    {
                        candidateFunctions.push_back(it->body->getSymbol());
                    }
                    else
                    {
                        auto id = symbol->id;
                        auto scope = symbol->owner;

                        auto clone = symbol->templated->body->clone(symbol->templated->body->getScope());
                        
                        symbol = scope->getSymbol(id);

                        for (int i = 0; i < symbol->templated->parameters.size(); ++i)
                        {
                            std::function<void(ASTNode* node)> checkOne;
                            checkOne = [&](ASTNode* node) {
                                node->setTemplateType(symbol->templated->parameters[i].type, var->mTemplateParameters[i]);
                                for (auto child : node->getContained())
                                {
                                    checkOne(child);
                                }
                            };
                            checkOne(clone.get());
                        }
                        clone->typeCheck(diag, exit);
                        candidateFunctions.push_back(clone->getSymbol());
                        symbol->templated->instantiations.push_back({std::move(clone), var->mTemplateParameters});
                    }
                }
                else
                {
                    candidateFunctions = mScope->getCandidateFunctions(var->getNames());
                }
            }
            else if (auto memberAccess = dynamic_cast<MemberAccess*>(mCallee.get()))
            {
                auto structType = memberAccess->mStructType;
                auto names = structType->getNames();
                names.push_back(memberAccess->mId);
                candidateFunctions = mScope->getCandidateFunctions(names);

                errorName = structType->getName();
                errorName += "::" + memberAccess->mId;

                mIsMemberFunction = true;
            }

            // Find all viable functions
            for (auto it = candidateFunctions.begin(); it != candidateFunctions.end();)
            {
                auto candidate = *it;
                if (!candidate->type->isFunctionType()) it = candidateFunctions.erase(it);
                else
                {
                    auto functionType = static_cast<FunctionType*>(candidate->type);
                    auto arguments = functionType->getArgumentTypes();
                    if (mIsMemberFunction) {if (arguments.size() != mParameters.size() + 1) candidateFunctions.erase(it); else ++it; }
                    else { if (arguments.size() != mParameters.size()) it = candidateFunctions.erase(it); else ++it; }
                }
            }

            std::vector<ViableFunction> viableFunctions;
            for (auto& candidate : candidateFunctions)
            {
                auto functionType = static_cast<FunctionType*>(candidate->type);
                int score = 0;
                bool disallowed = false;
                size_t i = mIsMemberFunction ? 1 : 0;
                for (;i < mParameters.size(); ++i)
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
                        fmt::bold, errorName, fmt::defaults)
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
                            fmt::bold, errorName, fmt::defaults)
                    );
                    return nullptr;
                }
            }
            return viableFunctions.front().symbol;
        }
        return nullptr;
    }
}