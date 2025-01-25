// Copyright 2025 solar-mist

#include "type/EnumType.h"

#include <format>

EnumType::EnumType(std::string name, Type* base)
    : Type(name)
    , mBase(base)
{
    if (std::isdigit(mName[0]))
    {
        std::vector<std::string> names;
        for (size_t i = 0; i < mName.size(); ++i)
        {
            std::string len;
            while (isdigit(mName[i]))
            {
                len += mName[i++];
            }
            if (len.empty()) continue;

            int size = std::stoi(len);
            names.emplace_back(&mName[i], &mName[i + size]);
        }
        for (auto it = names.begin(); it != names.end() - 1; ++it)
        {
            mFormattedName += *it + "::";
        }
        mFormattedName += names.back();
    }
    else
    {
        mFormattedName = mName;
    }
}

int EnumType::getSize() const
{
    return mBase->getSize();
}

Type::CastLevel EnumType::castTo(Type* destType) const
{
    if (mBase == destType) return Type::CastLevel::Explicit;
    if (mBase->castTo(destType) != Type::CastLevel::Disallowed) return Type::CastLevel::Explicit;
    return Type::CastLevel::Disallowed;
}

std::string EnumType::getMangleId() const
{
    return "_E" + mName;
}

vipir::Type* EnumType::getVipirType() const
{
    return vipir::Type::GetBooleanType();
}

std::string_view EnumType::getName()
{
    return mFormattedName;
}

EnumType* EnumType::Create(std::string name, Type* base)
{
    void AddType(std::string name, std::unique_ptr<Type> type);
    auto typePtr = std::make_unique<EnumType>(name, base);

    auto type = typePtr.get();

    AddType(name, std::move(typePtr));

    return type;
}