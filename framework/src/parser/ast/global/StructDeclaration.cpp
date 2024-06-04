// Copyright 2024 solar-mist


#include "parser/ast/global/StructDeclaration.h"

#include "type/StructType.h"
#include "type/PointerType.h"

#include "symbol/NameMangling.h"

#include <vipir/IR/BasicBlock.h>
#include <vipir/Type/FunctionType.h>
#include <vipir/IR/Instruction/GEPInst.h>
#include <vipir/IR/Instruction/PtrCastInst.h>

#include <vector>

#include "vipir/Type/PointerType.h"

namespace parser
{
    StructDeclaration::StructDeclaration(std::vector<std::string> names, std::vector<StructField> fields, std::vector<StructMethod> methods, Type* type)
        : mNames(std::move(names))
        , mFields(std::move(fields))
        , mMethods(std::move(methods))
    {
        mType = type;

        for (auto& method : mMethods)
        {
            std::vector<Type*> manglingArguments;
            std::vector<vipir::Type*> argumentTypes;

            manglingArguments.push_back(PointerType::Create(mType));
            argumentTypes.push_back(vipir::Type::GetPointerType(mType->getVipirType()));

            for (auto& argument : method.arguments)
            {
                manglingArguments.push_back(argument.type);
                argumentTypes.push_back(argument.type->getVipirType());
            }

            vipir::FunctionType* functionType = static_cast<vipir::FunctionType*>(method.type->getVipirType());

            std::vector<std::string> names = mNames;
            names.push_back(method.name);
            std::string name = symbol::mangleFunctionName(names, std::move(manglingArguments));

            FunctionSymbol::Create(nullptr, name, names, method.type, method.priv);
        }
    }

    std::vector<std::string>& StructDeclaration::getNames()
    {
        return mNames;
    }

    std::vector<StructField>& StructDeclaration::getFields()
    {
        return mFields;
    }

    std::vector<StructMethod>& StructDeclaration::getMethods()
    {
        return mMethods;
    }

    void StructDeclaration::typeCheck(Scope* scope, diagnostic::Diagnostics& diag)
    {
        for (auto& method : mMethods)
        {
            if (method.scope)
            {
                scope = method.scope.get();
                method.scope->currentReturnType = static_cast<FunctionType*>(method.type)->getReturnType();
            }
            for (auto& node : method.body)
            {
                node->typeCheck(scope, diag);
            }
        }
    }

    vipir::Value* StructDeclaration::emit(vipir::IRBuilder& builder, vipir::Module& module, Scope* scope, diagnostic::Diagnostics& diag)
    {
        StructType* structType = static_cast<StructType*>(mType);
        for (StructMethod& method : mMethods)
        {
            std::vector<Type*> manglingArguments;
            std::vector<vipir::Type*> argumentTypes;

            manglingArguments.push_back(PointerType::Create(mType));
            argumentTypes.push_back(vipir::Type::GetPointerType(mType->getVipirType()));

            for (auto& argument : method.arguments)
            {
                manglingArguments.push_back(argument.type);
                argumentTypes.push_back(argument.type->getVipirType());
            }

            vipir::FunctionType* functionType = static_cast<vipir::FunctionType*>(method.type->getVipirType());

            std::vector<std::string> names = mNames;
            names.push_back(method.name);
            std::string name = symbol::mangleFunctionName(names, std::move(manglingArguments));

            vipir::Function* func = vipir::Function::Create(functionType, module, name);
            GlobalFunctions[name].function = func;

            if (method.body.empty())
            {
                continue;
            }

            scope = method.scope.get();

            vipir::BasicBlock* entryBasicBlock = vipir::BasicBlock::Create("", func);
            builder.setInsertPoint(entryBasicBlock);

            int index = 0;

            vipir::AllocaInst* self = builder.CreateAlloca(vipir::Type::GetPointerType(mType->getVipirType()));
            scope->locals["this"].alloca = self;

            builder.CreateStore(self, func->getArgument(index++));

            for (auto& argument : method.arguments)
            {
                vipir::AllocaInst* alloca = builder.CreateAlloca(argument.type->getVipirType());
                scope->locals[argument.name].alloca = alloca;

                builder.CreateStore(alloca, func->getArgument(index++));
            }

            for (auto& node : method.body)
            {
                node->emit(builder, module, scope, diag);
            }
        }

        return nullptr;
    }
}
