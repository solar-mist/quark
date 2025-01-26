// Copyright 2024 solar-mist

#ifndef VIPER_FRAMEWORK_TYPE_POINTER_TYPE_H
#define VIPER_FRAMEWORK_TYPE_POINTER_TYPE_H 1

#include "type/Type.h"

class PointerType : public Type
{
public:
    PointerType(Type* returnType);

    Type* getPointeeType() const;

    virtual int getSize() const override;
    virtual vipir::Type* getVipirType() const override;
    virtual CastLevel castTo(Type* destType) const override;
    virtual std::string getMangleId() const override;
    virtual Type* replaceWith(Type* from, Type* to) override;

    bool isPointerType() const override;

    static PointerType* Get(Type* pointeeType);

private:
    Type* mPointeeType;
};

#endif // VIPER_FRAMEWORK_TYPE_POINTER_TYPE_H