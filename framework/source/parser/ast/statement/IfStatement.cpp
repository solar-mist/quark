// Copyright 2024 solar-mist

#include "parser/ast/statement/IfStatement.h"

#include <vipir/IR/BasicBlock.h>
#include <vipir/IR/Instruction/PhiInst.h>

namespace parser
{
    IfStatement::IfStatement(ASTNodePtr&& condition, ASTNodePtr&& body, ASTNodePtr&& elseBody, ScopePtr scope, lexer::Token token)
        : ASTNode(scope->parent, Type::Get("void"), token)
        , mCondition(std::move(condition))
        , mBody(std::move(body))
        , mElseBody(std::move(elseBody))
        , mOwnScope(std::move(scope))
    {
    }

    std::vector<ASTNode*> IfStatement::getContained() const
    {
        std::vector<ASTNode*> ret = {mBody.get(), mCondition.get()};
        if (mElseBody) ret.push_back(mElseBody.get());
        return ret;
    }

    ASTNodePtr IfStatement::clone(Scope* in)
    {
        auto scope = mOwnScope->clone(in);
        auto scopePtr = scope.get();
        return std::make_unique<IfStatement>(mCondition->clone(in), mBody->clone(scopePtr), mElseBody?mElseBody->clone(scopePtr):nullptr, std::move(scope), mErrorToken);
    }

    vipir::Value* IfStatement::codegen(vipir::IRBuilder& builder, vipir::Module& module, diagnostic::Diagnostics& diag)
    {
        vipir::Value* condition = mCondition->codegen(builder, module, diag);

        vipir::BasicBlock* startBasicBlock = builder.getInsertPoint();

        vipir::BasicBlock* falseBasicBlock;
        if (mElseBody)
        {
            falseBasicBlock = vipir::BasicBlock::Create("", builder.getInsertPoint()->getParent());
        }
        vipir::BasicBlock* trueBasicBlock = vipir::BasicBlock::Create("", builder.getInsertPoint()->getParent());

        vipir::BasicBlock* mergeBasicBlock = vipir::BasicBlock::Create("", builder.getInsertPoint()->getParent());

        trueBasicBlock->loopEnd() = mergeBasicBlock;
        if (mElseBody)
        {
            falseBasicBlock->loopEnd() = mergeBasicBlock;
            builder.CreateCondBr(condition, trueBasicBlock, falseBasicBlock);
        }
        else
        {
            builder.CreateCondBr(condition, trueBasicBlock, mergeBasicBlock);
        }

        builder.setInsertPoint(trueBasicBlock);
        mBody->codegen(builder, module, diag);
        builder.CreateBr(mergeBasicBlock);

        if (mElseBody)
        {
            builder.setInsertPoint(falseBasicBlock);
            mElseBody->codegen(builder, module, diag);
            builder.CreateBr(mergeBasicBlock);
        }

        builder.setInsertPoint(mergeBasicBlock);

        if (!mElseBody)
        {
            for (auto& symbol : mScope->symbols)
            {
                auto trueBasicBlockValue = symbol.getLatestValue(trueBasicBlock);
                auto startBasicBlockValue = symbol.getLatestValue(startBasicBlock);
                if (trueBasicBlockValue && trueBasicBlockValue != startBasicBlockValue)
                {
                    auto phi = builder.CreatePhi(symbol.type->getVipirType());
                    phi->addIncoming(trueBasicBlockValue, trueBasicBlock);
                    phi->addIncoming(startBasicBlockValue, startBasicBlock);

                    symbol.values.push_back(std::make_pair(mergeBasicBlock, phi));
                }
            }
        }
        else
        {
            for (auto& symbol : mScope->symbols)
            {
                auto trueBasicBlockValue = symbol.getLatestValue(trueBasicBlock);
                auto falseBasicBlockValue = symbol.getLatestValue(falseBasicBlock);
                if (trueBasicBlockValue != falseBasicBlockValue)
                {
                    if (trueBasicBlockValue == nullptr)
                    {
                        trueBasicBlockValue = symbol.getLatestValue(startBasicBlock);
                    }
                    else if (falseBasicBlockValue == nullptr)
                    {
                        falseBasicBlockValue = symbol.getLatestValue(startBasicBlock);
                    }

                    auto phi = builder.CreatePhi(symbol.type->getVipirType());
                    phi->addIncoming(trueBasicBlockValue, trueBasicBlock);
                    phi->addIncoming(falseBasicBlockValue, falseBasicBlock);

                    symbol.values.push_back(std::make_pair(mergeBasicBlock, phi));
                }
            }
        }

        return nullptr;
    }

    void IfStatement::semanticCheck(diagnostic::Diagnostics& diag, bool& exit, bool statement)
    {
        mCondition->semanticCheck(diag, exit, true);
        mBody->semanticCheck(diag, exit, false);
        if (mElseBody) mElseBody->semanticCheck(diag, exit, false);

        if (!statement)
        {
            diag.reportCompilerError(
                mErrorToken.getStartLocation(),
                mErrorToken.getEndLocation(),
                std::format("'{}if{}' statement used as an expression",
                    fmt::bold, fmt::defaults)
            );
        }
    }

    void IfStatement::typeCheck(diagnostic::Diagnostics& diag, bool& exit)
    {
        mCondition->typeCheck(diag, exit);
        mBody->typeCheck(diag, exit);
        if (mElseBody)
        {
            mElseBody->typeCheck(diag, exit);
        }

        if (!mCondition->getType()->isBooleanType())
        {
            auto boolType = Type::Get("bool");

            if (mCondition->implicitCast(diag, boolType))
            {
                mCondition = Cast(mCondition, boolType);
            }
            else
            {
                diag.reportCompilerError(
                    mCondition->getErrorToken().getStartLocation(),
                    mCondition->getErrorToken().getEndLocation(),
                    std::format("value of type '{}{}{}' cannot be used as a condition in if-statement",
                        fmt::bold, mCondition->getType()->getName(), fmt::defaults)
                );
            }
        }
    }

    bool IfStatement::triviallyImplicitCast(diagnostic::Diagnostics&, Type*)
    {
        return false;
    }
}