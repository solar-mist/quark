// Copyright 2025 solar-mist

#ifndef VIPER_FRAMEWORK_TYPE_TEMPLATE_TYPE_H
#define VIPER_FRAMEWORK_TYPE_TEMPLATE_TYPE_H 1

#include "type/Type.h"

class TemplateType : public Type
{
public:
    TemplateType(std::string name);

    virtual int getSize() const override;
    virtual vipir::Type* getVipirType() const override;
    virtual CastLevel castTo(Type* destType) const override;
    virtual std::string getMangleId() const override;

    virtual bool isTemplateType() const override;

    static TemplateType* Create(std::string name);
};

#endif // VIPER_FRAMEWORK_TYPE_TEMPLATE_TYPE_H