// Copyright 2025 solar-mist

#include "parser/ast/expression/MemberAccess.h"

#include "type/StructType.h"
#include "type/PointerType.h"

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

        if (statement)
        {
            diag.compilerWarning("unused", mErrorToken.getStartLocation(), mErrorToken.getEndLocation(), "expression result unused");
        }

        if (!mStructType->hasField(mId))
        {
            diag.reportCompilerError(mErrorToken.getStartLocation(), mErrorToken.getEndLocation(), std::format("'{}class {}{}' has no member named '{}{}{}'",
                fmt::bold, mStructType->getName(), fmt::defaults, fmt::bold, mId, fmt::defaults));
            exit = true;
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
            if (!static_cast<PointerType*>(mStruct->getType())->getPointeeType()->isStructType())
            {
                diag.reportCompilerError(mOperatorToken.getStartLocation(), mOperatorToken.getEndLocation(),
                    std::format("{}'operator->'{} used on non-pointer-to-struct value '{}{}{}'",
                        fmt::bold, fmt::defaults, fmt::bold, mStruct->getErrorToken().getText(), fmt::defaults));
                exit = true;
                mType = Type::Get("error-type");
                return;
            }
            mStructType = static_cast<StructType*>(static_cast<PointerType*>(mStruct->getType())->getPointeeType());
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
            mStructType = static_cast<StructType*>(mStruct->getType());
        }

        auto structField = mStructType->getField(mId);
        if (structField)
            mType = structField->type;
        else
        {
            diag.reportCompilerError(mErrorToken.getStartLocation(), mErrorToken.getEndLocation(), std::format("'{}class {}{}' has no member named '{}{}{}'",
                fmt::bold, mStructType->getName(), fmt::defaults, fmt::bold, mId, fmt::defaults));
            exit = true;
            mType = Type::Get("error-type");
        }
    }

    bool MemberAccess::triviallyImplicitCast(diagnostic::Diagnostics&, Type*)
    {
        return false;
    }
}