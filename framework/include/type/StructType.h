// Copyright 2025 solar-mist

#ifndef VIPER_FRAMEWORK_TYPE_STRUCT_TYPE_H
#define VIPER_FRAMEWORK_TYPE_STRUCT_TYPE_H 1

#include "type/Type.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

class StructType : public Type
{
public:
    struct Field
    {
        bool priv;
        std::string name;
        Type* type;
    };

    StructType(std::string name, std::vector<Field> fields);

    std::string_view getName() const;

    std::vector<Field>& getFields();
    bool hasField(std::string_view fieldName);
    Field* getField(std::string_view fieldName);
    int getFieldOffset(std::string fieldName);

    virtual int getSize() const override;
    virtual vipir::Type* getVipirType() const override;
    virtual CastLevel castTo(Type* destType) const override;
    virtual std::string getMangleId() const override;

    bool isStructType() const override;

    static StructType* Get(std::string name);
    static StructType* Create(std::string name, std::vector<Field> fields);
    static void Erase(Type* type);

private:
    std::string mName;
    std::vector<Field> mFields;
};

#endif // VIPER_FRAMEWORK_TYPE_STRUCT_TYPE_H