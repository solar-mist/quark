// Copyright 2024 solar-mist

#include "type/TemplateType.h"

#include <format>

TemplateType::TemplateType(std::string name)
    : Type(name)
{
}

int TemplateType::getSize() const
{
    return 0;
}

Type::CastLevel TemplateType::castTo(Type* destType) const
{
    return Type::CastLevel::Disallowed;
}

std::string TemplateType::getMangleId() const
{
    return "STRAY TEMPLATETYPE IN PROGRAM";
}

vipir::Type* TemplateType::getVipirType() const
{
    return nullptr;
}

bool TemplateType::isTemplateType() const
{
    return true;
}

static std::vector<TemplateType> templateTypes;

TemplateType* TemplateType::Create(std::string name)
{
    templateTypes.emplace_back(name);
    return &templateTypes.back();
}