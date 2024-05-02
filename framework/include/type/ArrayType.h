// Copyright 2024 solar-mist

#ifndef VIPER_FRAMEWORK_TYPE_ARRAY_TYPE_H
#define VIPER_FRAMEWORK_TYPE_ARRAY_TYPE_H 1

#include "type/Type.h"

class ArrayType : public Type
{
public:
    ArrayType(Type* base, int count);

    Type* getBaseType() const;

    int getSize() const override;
    vipir::Type* getVipirType() const override;

    bool isArrayType() const override;

    static ArrayType* Create(Type* base, int count);

private:
    Type* mBase;
    int mCount;
};

#endif // VIPER_FRAMEWORK_TYPE_ARRAY_TYPE_H