// Copyright 2024 solar-mist


#include "type/EnumType.h"

#include <algorithm>
#include <unordered_map>

EnumType::EnumType(std::vector<std::string> names, bool generatedNames)
    : Type(names.back())
    , mNames(std::move(names))
    , mGeneratedNames(generatedNames)
{
}

bool EnumType::hsGeneratedNames() const
{
    return mGeneratedNames;
}


int EnumType::getSize() const
{
    return 32;
}

vipir::Type* EnumType::getVipirType() const
{
    return vipir::Type::GetIntegerType(32);
}

std::string EnumType::getMangleID() const
{
    std::string ret = "_E";
    for (auto& name : mNames)
    {
        ret += std::to_string(name.length());
        ret += name;
    }

    return ret;
}

bool EnumType::isEnumType() const
{
    return true;
}

extern std::unordered_map<std::string, std::unique_ptr<Type>> types;
EnumType* EnumType::Create(std::vector<std::string> names, bool generatedNames)
{
    auto it = std::find_if(types.begin(), types.end(), [&names](const auto& type){
        if (EnumType* enumType = dynamic_cast<EnumType*>(type.second.get())) {
            return enumType->mNames == names;
        }
        return false;
    });

    if (it != types.end()) {
        return static_cast<EnumType*>(it->second.get());
    }

    std::unique_ptr<Type> type = std::make_unique<EnumType>(names, generatedNames);
    std::string mangleID = type->getMangleID();
    types[mangleID] = std::move(type);

    return static_cast<EnumType*>(types[mangleID].get());
}