// Copyright 2025 solar-mist

#ifndef VIPER_FRAMEWORK_TYPE_STRUCT_TYPE_H
#define VIPER_FRAMEWORK_TYPE_STRUCT_TYPE_H 1

#include "type/Type.h"

#include "lexer/Token.h"

#include <map>
#include <string>
#include <utility>
#include <variant>
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

    static std::string MangleName(std::vector<std::string>& names);

private:
    std::string mName;
    std::vector<Field> mFields;
};

class IncompleteStructType : public Type
{
public:
    IncompleteStructType(int size);

    virtual int getSize() const override;
    virtual vipir::Type* getVipirType() const override;
    virtual CastLevel castTo(Type* destType) const override;
    virtual std::string getMangleId() const override;
    
    bool isObjectType() const override;

    static void Create(std::string name, int size);

private:
    int mSize;
};

class PendingStructType : public Type
{
public:
    PendingStructType(lexer::Token token, std::string name, std::vector<StructType::Field> fields);

    virtual int getSize() const override;
    virtual vipir::Type* getVipirType() const override;
    virtual CastLevel castTo(Type* destType) const override;
    virtual std::string getMangleId() const override;

    bool isStructType() const override;
    bool isObjectType() const override;

    void initComplete();
    void initIncomplete();
    void setFields(std::vector<StructType::Field> fields);
    
    StructType* get();
    lexer::Token& getToken();

    static PendingStructType* Create(lexer::Token token, std::string name, std::vector<StructType::Field> fields);
    static std::vector<PendingStructType*> GetPending();

private:
    lexer::Token mToken;
    std::variant<std::monostate, StructType, IncompleteStructType> mImpl;
    std::vector<StructType::Field> mFields;
};

#endif // VIPER_FRAMEWORK_TYPE_STRUCT_TYPE_H