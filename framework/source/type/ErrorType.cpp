// Copyright 2024 solar-mist

#include "type/ErrorType.h"

ErrorType::ErrorType()
    : Type("error-type")
{
}

int ErrorType::getSize() const
{
    return 0;
}

Type::CastLevel ErrorType::castTo(Type* destType) const
{
    return CastLevel::Disallowed;
}

vipir::Type* ErrorType::getVipirType() const
{
    return vipir::Type::GetVoidType();
}

std::string ErrorType::getMangleId() const
{
    return "Stray error-type in program";
}

bool ErrorType::isObjectType() const
{
    return false;
}