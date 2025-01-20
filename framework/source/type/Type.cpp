// Copyright 2024 solar-mist

#include "type/Type.h"
#include "type/IntegerType.h"
#include "type/VoidType.h"
#include "type/BooleanType.h"
#include "type/ErrorType.h"

#include <unordered_map>

std::unordered_map<std::string, std::unique_ptr<Type>> types;

void AddType(std::string name, std::unique_ptr<Type> type)
{
    types[name] = std::move(type);
}

void Type::Init()
{
    types["i8"]  = std::make_unique<IntegerType>(8, true);
    types["i16"] = std::make_unique<IntegerType>(16, true);
    types["i32"] = std::make_unique<IntegerType>(32, true);
    types["i64"] = std::make_unique<IntegerType>(64, true);
    types["u8"]  = std::make_unique<IntegerType>(8, false);
    types["u16"] = std::make_unique<IntegerType>(16, false);
    types["u32"] = std::make_unique<IntegerType>(32, false);
    types["u64"] = std::make_unique<IntegerType>(64, false);

    types["void"] = std::make_unique<VoidType>();
    types["bool"] = std::make_unique<BooleanType>();
    types["error-type"] = std::make_unique<ErrorType>();
}

bool Type::Exists(const std::string& name)
{
    auto type = types.find(name);
    return type != types.end();
}

Type* Type::Get(const std::string& name)
{
    auto type = types.find(name);
    if (type != types.end()) return type->second.get();

    return nullptr;
}