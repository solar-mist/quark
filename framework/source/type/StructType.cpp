// Copyright 2025 solar-mist


#include "type/StructType.h"
#include "type/PointerType.h"

#include <vipir/Type/StructType.h>
#include <vipir/Type/PointerType.h>

#include <algorithm>
#include <map>
#include <vector>

StructType::StructType(std::string name, std::vector<Field> fields)
    : Type(name)
    , mName(std::move(name))
    , mFields(std::move(fields))
{
}

std::string_view StructType::getName() const
{
    return mName;
}

std::vector<StructType::Field>& StructType::getFields()
{
    return mFields;
}

bool StructType::hasField(std::string_view fieldName)
{
    return std::find_if(mFields.begin(), mFields.end(), [&fieldName](const Field& field){
        return fieldName == field.name;
    }) != mFields.end();
}

StructType::Field* StructType::getField(std::string_view fieldName)
{
    auto it = std::find_if(mFields.begin(), mFields.end(), [&fieldName](const Field& field){
        return fieldName == field.name;
    });
    if (it == mFields.end()) return nullptr;

    return &*it;
}

int StructType::getFieldOffset(std::string fieldName)
{
    return std::find_if(mFields.begin(), mFields.end(), [&fieldName](const Field& field){
        return fieldName == field.name;
    }) - mFields.begin();
}

int StructType::getSize() const
{
    int size = 0;
    for (auto& field : mFields)
        size += field.type->getSize();
    
    return size;
}

vipir::Type* StructType::getVipirType() const
{
    std::vector<vipir::Type*> fieldTypes;
    for (auto [_, _x, field] : mFields)
    {
        if (field->isPointerType())
        {
            // struct types with a pointer to themselves cannot be emitted normally
            if (static_cast<PointerType*>(field)->getPointeeType() == this)
            {
                fieldTypes.push_back(vipir::PointerType::GetPointerType(vipir::Type::GetIntegerType(8)));
                continue;
            }
        }
        fieldTypes.push_back(field->getVipirType());
    }
    return vipir::Type::GetStructType(std::move(fieldTypes));
}

Type::CastLevel StructType::castTo(Type*) const
{
    // TODO: Look up cast operators?
    return CastLevel::Disallowed;
}

std::string StructType::getMangleId() const
{
    std::string ret = "S";
    ret += std::to_string(mName.length()) + mName;
    return ret;
}

bool StructType::isStructType() const
{
    return true;
}


static std::vector<std::unique_ptr<StructType> > structTypes;

StructType* StructType::Get(std::string name)
{
    auto it = std::find_if(structTypes.begin(), structTypes.end(), [&name](const auto& type){
        return type->getName() == name;
    });
    if (it == structTypes.end()) return nullptr;
    return it->get();
}

StructType* StructType::Create(std::string name, std::vector<StructType::Field> fields)
{
    auto it = std::find_if(structTypes.begin(), structTypes.end(), [&name](const auto& type){
        return type->mName == name;
    });

    if (it != structTypes.end())
    {
        return it->get();
    }

    structTypes.push_back(std::make_unique<StructType>(name, std::move(fields)));
    return structTypes.back().get();
}

void StructType::Erase(Type* type)
{
    auto structType = static_cast<StructType*>(type);
    
    structTypes.erase(std::remove_if(structTypes.begin(), structTypes.end(), [structType](const auto& type){
        return type.get() == structType;
    }), structTypes.end());
}