// Copyright 2025 solar-mist

#ifndef VIPER_FRAMEWORK_TYPE_ENUM_TYPE_H
#define VIPER_FRAMEWORK_TYPE_ENUM_TYPE_H 1

#include "type/Type.h"

class EnumType : public Type
{
public:
    EnumType(std::string name, Type* base);

    virtual int getSize() const override;
    virtual vipir::Type* getVipirType() const override;
    virtual CastLevel castTo(Type* destType) const override;
    virtual std::string getMangleId() const override;

    virtual std::string_view getName() override;

    static EnumType* Create(std::string name, Type* base);

private:
    Type* mBase;
    std::string mFormattedName;
};

#endif // VIPER_FRAMEWORK_TYPE_ENUM_TYPE_H