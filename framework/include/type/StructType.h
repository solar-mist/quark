// Copyright 2024 solar-mist

#ifndef VIPER_FRAMEWORK_TYPE_STRUCT_TYPE_H
#define VIPER_FRAMEWORK_TYPE_STRUCT_TYPE_H 1

#include "type/Type.h"

#include <map>

class StructType : public Type
{
public:
    struct Field
    {
        std::string name;
        Type* type;
    };

    StructType(std::vector<Field> fields);

    const std::vector<Field>& getFields() const;
    Field& getField(std::string fieldName);
    int getFieldOffset(std::string fieldName);

    int getSize() const override;
    vipir::Type* getVipirType() const override;

    bool isPointerType() const override;

    static StructType* Get(std::string name);
    static StructType* Create(std::string name, std::vector<Field> fields);

private:
    std::vector<Field> mFields;
};

#endif // VIPER_FRAMEWORK_TYPE_STRUCT_TYPE_H