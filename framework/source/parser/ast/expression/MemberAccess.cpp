// Copyright 2025 solar-mist

#include "parser/ast/expression/MemberAccess.h"

#include "type/StructType.h"
#include "type/PointerType.h"
#include "type/FunctionType.h"

#include <vipir/IR/Instruction/GEPInst.h>
#include <vipir/IR/Instruction/LoadInst.h>
#include <vipir/IR/Instruction/PtrCastInst.h>

#include <vipir/Type/PointerType.h>

#include <vipir/Module.h>

#include <iostream>

namespace parser
{
    MemberAccess::MemberAccess(ASTNodePtr structNode, std::string id, bool pointer, Scope* scope, lexer::Token operatorToken, lexer::Token fieldToken)
        : ASTNode(scope, std::move(fieldToken))
        , mStruct(std::move(structNode))
        , mId(id)
        , mPointer(pointer)
        , mOperatorToken(std::move(operatorToken))
    {
    }

    std::vector<ASTNode*> MemberAccess::getContained() const
    {
        return {mStruct.get()};
    }

    ASTNodePtr MemberAccess::clone(Scope* in)
    {
        return std::make_unique<MemberAccess>(mStruct->clone(in), mId, mPointer, in, mOperatorToken, mErrorToken);
    }

    vipir::Value* MemberAccess::codegen(vipir::IRBuilder& builder, vipir::Module& module, diagnostic::Diagnostics& diag)
    {
        vipir::Value* struc;
        if (mPointer)
        {
            struc = mStruct->codegen(builder, module, diag);
        }
        else
        {
            vipir::Value* structValue = mStruct->codegen(builder, module, diag);
            struc = vipir::getPointerOperand(structValue);

            vipir::Instruction* instruction = static_cast<vipir::Instruction*>(structValue);
            instruction->eraseFromParent();
        }

        vipir::Value* gep = builder.CreateStructGEP(struc, mStructType->getFieldOffset(mId));

        // struct types with a pointer to themselves cannot be emitted normally
        if (mStructType->getField(mId)->type->isPointerType())
        {
            if (static_cast<PointerType*>(mStructType->getField(mId)->type)->getPointeeType() == mStructType)
            {
                vipir::Type* type = vipir::PointerType::GetPointerType(vipir::PointerType::GetPointerType(mStructType->getVipirType()));
                gep = builder.CreatePtrCast(gep, type);
            }
        }

        return builder.CreateLoad(gep);
    }

    void MemberAccess::semanticCheck(diagnostic::Diagnostics& diag, bool& exit, bool statement)
    {
        mStruct->semanticCheck(diag, exit, statement);

        auto structField = mStructType->getField(mId);

        auto scopeOwner = mScope->findOwner();
        StructType* structType = nullptr;
        
        if ((structType = dynamic_cast<StructType*>(scopeOwner)));
        else if (auto pending = dynamic_cast<PendingStructType*>(scopeOwner))
            structType = pending->get();

        if (structField && structField->priv && mStructType != structType)
        {
            diag.reportCompilerError(mErrorToken.getStartLocation(), mErrorToken.getEndLocation(), std::format("'{}{}{}' is a private member of class '{}{}{}",
                fmt::bold, mId, fmt::defaults,
                fmt::bold, mStructType->getName(), fmt::defaults
            ));
            exit = true;
        }
        auto structMethod = mStructType->getMethod(mId);
        if (structMethod && structMethod->priv && mStructType != structType)
        {
            diag.reportCompilerError(mErrorToken.getStartLocation(), mErrorToken.getEndLocation(), std::format("'{}{}{}' is a private member of class '{}{}{}",
                fmt::bold, mId, fmt::defaults,
                fmt::bold, mStructType->getName(), fmt::defaults
            ));
            exit = true;
        }

        if (statement)
        {
            diag.compilerWarning("unused", mErrorToken.getStartLocation(), mErrorToken.getEndLocation(), "expression result unused");
        }
    }

    void MemberAccess::typeCheck(diagnostic::Diagnostics& diag, bool& exit)
    {
        mStruct->typeCheck(diag, exit);

        if (mPointer)
        {
            if (!mStruct->getType()->isPointerType())
            {
                diag.reportCompilerError(mOperatorToken.getStartLocation(), mOperatorToken.getEndLocation(),
                    std::format("{}'operator->'{} used on non-pointer value '{}{}{}'",
                        fmt::bold, fmt::defaults, fmt::bold, mStruct->getErrorToken().getText(), fmt::defaults));
                // TODO: Add note suggesting use of non-pointer member access operator
                exit = true;
                mType = Type::Get("error-type");
                return;
            }
            auto pointeeType = static_cast<PointerType*>(mStruct->getType())->getPointeeType();
            if (!pointeeType->isStructType())
            {
                diag.reportCompilerError(mOperatorToken.getStartLocation(), mOperatorToken.getEndLocation(),
                    std::format("{}'operator->'{} used on non-pointer-to-struct value '{}{}{}'",
                        fmt::bold, fmt::defaults, fmt::bold, mStruct->getErrorToken().getText(), fmt::defaults));
                exit = true;
                mType = Type::Get("error-type");
                return;
            }
            if (auto pending = dynamic_cast<PendingStructType*>(pointeeType))
                mStructType = pending->get();
            else
                mStructType = static_cast<StructType*>(pointeeType);
        }
        else
        {
            if (!mStruct->getType()->isStructType())
            {
                diag.reportCompilerError(mOperatorToken.getStartLocation(), mOperatorToken.getEndLocation(),
                    std::format("{}'operator.'{} used on non-struct value '{}{}{}'",
                        fmt::bold, fmt::defaults, fmt::bold, mStruct->getErrorToken().getText(), fmt::defaults));
                exit = true;
                mType = Type::Get("error-type");
                return;
            }
            if (auto pending = dynamic_cast<PendingStructType*>(mStruct->getType()))
                mStructType = pending->get();
            else
                mStructType = static_cast<StructType*>(mStruct->getType());
        }

        auto structField = mStructType->getField(mId);
        if (structField)
            mType = structField->type;
        else
        {
            auto structMethod = mStructType->getMethod(mId);
            if (structMethod)
            {
                auto functionType = static_cast<FunctionType*>(structMethod->type);
                mType = functionType->getReturnType();
            }
            else
            {
                diag.reportCompilerError(mErrorToken.getStartLocation(), mErrorToken.getEndLocation(), std::format("class '{}{}{}' has no member named '{}{}{}'",
                    fmt::bold, mStructType->getName(), fmt::defaults, fmt::bold, mId, fmt::defaults));
                exit = true;
                mType = Type::Get("error-type");
            }
        }
    }

    bool MemberAccess::triviallyImplicitCast(diagnostic::Diagnostics&, Type*)
    {
        return false;
    }
}