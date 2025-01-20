// Copyright 2024 solar-mist

#ifndef VIPER_FRAMEWORK_TYPE_ERROR_TYPE_H
#define VIPER_FRAMEWORK_TYPE_ERROR_TYPE_H 1

#include "type/Type.h"

class ErrorType : public Type
{
public:
    ErrorType();

    virtual int getSize() const override;
    virtual vipir::Type* getVipirType() const override;
    virtual CastLevel castTo(Type* destType) const override;
    virtual std::string getMangleId() const override;
    
    bool isObjectType() const override;
};

#endif // VIPER_FRAMEWORK_TYPE_ERROR_TYPE_H