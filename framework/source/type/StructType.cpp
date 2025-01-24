// Copyright 2025 solar-mist


#include "type/StructType.h"
#include "type/PointerType.h"

#include <vipir/Type/StructType.h>
#include <vipir/Type/PointerType.h>

#include <algorithm>
#include <map>
#include <sstream>
#include <vector>

StructType::StructType(std::string name, std::vector<Field> fields, std::vector<Method> methods)
    : Type(name)
    , mName(std::move(name))
    , mFields(std::move(fields))
    , mMethods(std::move(methods))
{
    if (std::isdigit(mName[0]))
    {
        for (size_t i = 0; i < mName.size(); ++i)
        {
            std::string len;
            while (isdigit(mName[i]))
            {
                len += mName[i++];
            }
            if (len.empty()) continue;

            int size = std::stoi(len);
            mNames.emplace_back(&mName[i], &mName[i + size]);
        }
        for (auto it = mNames.begin(); it != mNames.end() - 1; ++it)
        {
            mFormattedName += *it + "::";
        }
        mFormattedName += mNames.back();
    }
    else
    {
        mFormattedName = mName;
    }
}

std::string_view StructType::getName() const
{
    return mName;
}

std::vector<std::string> StructType::getNames()
{
    return mNames;
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

StructType::Method* StructType::getMethod(std::string_view methodName)
{
    auto it = std::find_if(mMethods.begin(), mMethods.end(), [&methodName](const Method& method){
        return methodName == method.name;
    });
    if (it == mMethods.end()) return nullptr;

    return &*it;
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

std::string_view StructType::getName()
{
    return mFormattedName;
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

StructType* StructType::Create(std::string name, std::vector<StructType::Field> fields, std::vector<Method> methods)
{
    auto it = std::find_if(structTypes.begin(), structTypes.end(), [&name](const auto& type){
        return type->mName == name;
    });

    if (it != structTypes.end())
    {
        return it->get();
    }

    structTypes.push_back(std::make_unique<StructType>(name, std::move(fields), std::move(methods)));
    return structTypes.back().get();
}

void StructType::Erase(Type* type)
{
    auto structType = static_cast<StructType*>(type);
    
    structTypes.erase(std::remove_if(structTypes.begin(), structTypes.end(), [structType](const auto& type){
        return type.get() == structType;
    }), structTypes.end());
}

std::string StructType::MangleName(std::vector<std::string>& names)
{
    int trueCount = 0;
    std::string ret;
    for (auto& name : names)
    {
        if (name.empty()) continue; // Ignore empty namespace names
        ++trueCount;
        ret += std::to_string(name.length());
        ret += name;
    }
    if (trueCount == 1) return names.back();
    return ret;
}


IncompleteStructType::IncompleteStructType(int size)
    : Type("error-type")
    , mSize(size)
{
}

int IncompleteStructType::getSize() const
{
    return mSize;
}

vipir::Type* IncompleteStructType::getVipirType() const
{
    return vipir::Type::GetVoidType();
}

Type::CastLevel IncompleteStructType::castTo(Type* destType) const
{
    return CastLevel::Disallowed;
}

std::string IncompleteStructType::getMangleId() const
{
    return "Stray error-type in program";
}

bool IncompleteStructType::isObjectType() const
{
    return false;
}

void IncompleteStructType::Create(std::string name, int size)
{
    void AddType(std::string name, std::unique_ptr<Type> type);

    AddType(name, std::make_unique<IncompleteStructType>(size));
}

// helper type for the visitor #4
template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };
// explicit deduction guide (not needed as of C++20)
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

static std::vector<PendingStructType*> pendings;

PendingStructType::PendingStructType(lexer::Token token, std::string name, std::vector<StructType::Field> fields, std::vector<StructType::Method> methods)
    : Type(name)
    , mToken(std::move(token))
    , mFields(std::move(fields))
    , mMethods(std::move(methods))
{
}

int PendingStructType::getSize() const
{
    int ret;
    for (auto& field : mFields)
    {
        ret += field.type->getSize();
    }
    return ret;
}

vipir::Type* PendingStructType::getVipirType() const
{
    vipir::Type* ret;
    std::visit(overloaded{
        [&ret](auto arg) { ret = arg.getVipirType(); },
        [](std::monostate arg) {}
    }, mImpl);
    return ret;
}

Type::CastLevel PendingStructType::castTo(Type* type) const
{
    Type::CastLevel ret;
    std::visit(overloaded{
        [&ret, type](auto arg) { ret = arg.castTo(type); },
        [](std::monostate arg) {}
    }, mImpl);
    return ret;
}

std::string PendingStructType::getMangleId() const
{
    std::string ret;
    std::visit(overloaded{
        [&ret](auto arg) { ret = arg.getMangleId(); },
        [](std::monostate arg) {}
    }, mImpl);
    return ret;
}

std::string_view PendingStructType::getName()
{
    std::string_view ret;
    std::visit(overloaded{
        [&ret](auto arg) { ret = arg.getName(); },
        [](std::monostate arg) {}
    }, mImpl);
    return ret;
}

bool PendingStructType::isStructType() const
{
    bool ret;
    std::visit(overloaded{
        [&ret](StructType arg) { ret = true; },
        [&ret](auto arg) { ret = false; }
    }, mImpl);
    return ret;
}

bool PendingStructType::isObjectType() const
{
    bool ret;
    std::visit(overloaded{
        [&ret](StructType arg) { ret = true; },
        [&ret](auto arg) { ret = false; }
    }, mImpl);
    return ret;
}

void PendingStructType::initComplete()
{
    mImpl = StructType(mName, mFields, mMethods);
    std::erase(pendings, this);
}

void PendingStructType::initIncomplete()
{
    mImpl = IncompleteStructType(getSize());
    std::erase(pendings, this);
}

void PendingStructType::set(std::vector<StructType::Field> fields, std::vector<StructType::Method> methods)
{
    mImpl = std::monostate();
    mFields = std::move(fields);
    mMethods = std::move(methods);
    pendings.push_back(this);
}

StructType* PendingStructType::get()
{
    return &std::get<1>(mImpl);
}

lexer::Token& PendingStructType::getToken()
{
    return mToken;
}

PendingStructType* PendingStructType::Create(lexer::Token token, std::string name, std::vector<StructType::Field> fields, std::vector<StructType::Method> methods)
{
    void AddType(std::string name, std::unique_ptr<Type> type);
    auto typePtr = std::make_unique<PendingStructType>(std::move(token), name, std::move(fields), std::move(methods));

    auto type = typePtr.get();
    pendings.push_back(type);

    AddType(name, std::move(typePtr));

    return type;
}

std::vector<PendingStructType*>& PendingStructType::GetPending()
{
    return pendings;
}