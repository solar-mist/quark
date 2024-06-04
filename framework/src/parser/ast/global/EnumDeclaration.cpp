// Copyright 2024 solar-mist


#include "parser/ast/global/EnumDeclaration.h"

#include "symbol/Identifier.h"

#include "type/EnumType.h"

#include <vipir/IR/Constant/ConstantInt.h>
#include <vipir/Module.h>

#include <algorithm>

namespace parser
{
    EnumDeclaration::EnumDeclaration(std::vector<GlobalAttribute> attributes, std::vector<std::string> names, std::vector<EnumField> fields)
        : mAttributes(std::move(attributes))
        , mNames(std::move(names))
        , mFields(std::move(fields))
    {
        bool generateNames = std::find_if(mAttributes.begin(), mAttributes.end(), [](const auto& attribute){
            return attribute.getType() == GlobalAttributeType::GenerateNames;
        }) == mAttributes.end();
        mType = EnumType::Create(mNames);
        symbol::AddIdentifier(mType->getMangleID(), mNames);

        for (auto& field : mFields)
        {
            std::string mangledName = "_EM" + field.name;

            std::vector<std::string> names = mNames;
            names.push_back(field.name);

            symbol::AddIdentifier(mangledName, std::move(names));

            GlobalVariables[mangledName] = GlobalSymbol(nullptr, mType);
        }
    }

    void EnumDeclaration::typeCheck(Scope* scope, diagnostic::Diagnostics& diag)
    {
    }

    vipir::Value* EnumDeclaration::emit(vipir::IRBuilder& builder, vipir::Module& module, Scope* scope, diagnostic::Diagnostics& diag)
    {
        for (auto& field : mFields)
        {
            std::string mangledName = "_EM" + field.name;

            vipir::Value* constant = vipir::ConstantInt::Get(module, field.value, vipir::Type::GetIntegerType(32));
            GlobalVariables[mangledName] = GlobalSymbol(constant, mType);
        }

        return nullptr;
    }
}