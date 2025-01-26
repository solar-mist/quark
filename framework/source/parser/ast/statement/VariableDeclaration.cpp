// Copyright 2024 solar-mist

#include "parser/ast/statement/VariableDeclaration.h"

#include <vipir/IR/Instruction/AllocaInst.h>

#include <algorithm>

namespace parser
{
    VariableDeclaration::VariableDeclaration(Scope* scope, std::string name, Type* type, ASTNodePtr initValue, lexer::Token token)
        : ASTNode(scope, type, token)
        , mName(std::move(name))
        , mInitValue(std::move(initValue))
    {
        mScope->symbols.emplace_back(mName, mType, mScope);
    }

    std::vector<ASTNode*> VariableDeclaration::getContained() const
    {
        if (mInitValue) return {mInitValue.get()};
        return {};
    }

    ASTNodePtr VariableDeclaration::clone(Scope* in)
    {
        return std::make_unique<VariableDeclaration>(in, mName, mType, mInitValue?mInitValue->clone(in):0, mErrorToken);
    }

    vipir::Value* VariableDeclaration::codegen(vipir::IRBuilder& builder, vipir::Module& module, diagnostic::Diagnostics& diag)
    {
        if (mType->isStructType())
        {
            auto it = std::find_if(mScope->symbols.begin(), mScope->symbols.end(), [this](const auto& symbol){
                return symbol.name == mName;
            });
            vipir::Value* alloca = builder.CreateAlloca(mType->getVipirType());
            it->values.push_back(std::make_pair(builder.getInsertPoint(), alloca));

            if (mInitValue)
            {
                throw std::runtime_error("Unimplemented struct initialization");
            }
        }
        else
        {
            if (mInitValue)
            {
                vipir::Value* initValue = mInitValue->codegen(builder, module, diag);
                auto it = std::find_if(mScope->symbols.begin(), mScope->symbols.end(), [this](const auto& symbol){
                    return symbol.name == mName;
                });
                it->values.push_back(std::make_pair(builder.getInsertPoint(), initValue));
            }
        }

        return nullptr;
    }

    void VariableDeclaration::semanticCheck(diagnostic::Diagnostics& diag, bool& exit, bool statement)
    {
        if (mInitValue) mInitValue->semanticCheck(diag, exit, false);

        if (!statement)
        {
            diag.reportCompilerError(
                mErrorToken.getStartLocation(),
                mErrorToken.getEndLocation(),
                std::format("'{}return{}' statement used as an expression",
                    fmt::bold, fmt::defaults)
            );
        }
    }
    
    void VariableDeclaration::typeCheck(diagnostic::Diagnostics& diag, bool& exit)
    {
        if (!mType)
        {
            if (!mInitValue)
            {
                diag.reportCompilerError(mErrorToken.getStartLocation(), mErrorToken.getEndLocation(), std::format("object '{}{}{}' has unknown type",
                    fmt::bold, mErrorToken.getText(), fmt::defaults
                ));
                exit = true;
                mType = Type::Get("error-type");
                return;
            }

            mInitValue->typeCheck(diag, exit);
            mType = mInitValue->getType();
            auto it = std::find_if(mScope->symbols.begin(), mScope->symbols.end(), [this](const auto& symbol){
                return symbol.name == mName;
            });
            it->type = mType; // This needs to be set again as it was set to null in the constructor
        }

        if (!mType->isObjectType())
        {
            diag.reportCompilerError(mErrorToken.getStartLocation(), mErrorToken.getEndLocation(), std::format("may not create object of type '{}{}{}'",
                fmt::bold, mType->getName(), fmt::defaults
            ));
            exit = true;
            mType = Type::Get("error-type");
            return;
        }

        if (mInitValue)
        {
            mInitValue->typeCheck(diag, exit);

            if (mInitValue->getType() != mType)
            {
                if (mInitValue->implicitCast(diag, mType))
                {
                    mInitValue = Cast(mInitValue, mType);
                }
                else
                {
                    diag.reportCompilerError(
                        mInitValue->getErrorToken().getStartLocation(),
                        mInitValue->getErrorToken().getEndLocation(),
                        std::format("value of type '{}{}{}' is not compatible with variable of type '{}{}{}'",
                            fmt::bold, mInitValue->getType()->getName(), fmt::defaults,
                            fmt::bold, mType->getName(), fmt::defaults)
                    );
                    exit = true;
                }
            }
        }
    }

    bool VariableDeclaration::triviallyImplicitCast(diagnostic::Diagnostics&, Type*)
    {
        return false;
    }
}