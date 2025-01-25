// Copyright 2025 solar-mist

#ifndef VIPER_FRAMEWORK_TYPE_STRUCT_TYPE_H
#define VIPER_FRAMEWORK_TYPE_STRUCT_TYPE_H 1

#include "type/Type.h"
#include "type/EnumType.h"

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

    struct Method
    {
        bool priv;
        std::string name;
        Type* type;
    };

    StructType(std::string name, std::vector<Field> fields, std::vector<Method> methods);

    std::string_view getName() const;
    std::vector<std::string> getNames();

    std::vector<Field>& getFields();
    bool hasField(std::string_view fieldName);
    Field* getField(std::string_view fieldName);
    int getFieldOffset(std::string fieldName);

    Method* getMethod(std::string_view methodName);

    virtual int getSize() const override;
    virtual vipir::Type* getVipirType() const override;
    virtual CastLevel castTo(Type* destType) const override;
    virtual std::string getMangleId() const override;

    virtual std::string_view getName() override;

    bool isStructType() const override;

    static StructType* Get(std::string name);
    static StructType* Create(std::string name, std::vector<Field> fields, std::vector<Method> methods);
    static void Erase(Type* type);

    static std::string MangleName(std::vector<std::string>& names);

private:
    std::string mName;
    std::string mFormattedName;
    std::vector<std::string> mNames;
    std::vector<Field> mFields;
    std::vector<Method> mMethods;
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
    PendingStructType(lexer::Token token, std::string name, std::vector<StructType::Field> fields, std::vector<StructType::Method> methods); // For classes
    PendingStructType(lexer::Token token, std::string name, Type* base); // For enums

    virtual int getSize() const override;
    virtual vipir::Type* getVipirType() const override;
    virtual CastLevel castTo(Type* destType) const override;
    virtual std::string getMangleId() const override;

    virtual std::string_view getName() override;

    bool isStructType() const override;
    bool isEnumType()   const override;
    bool isObjectType() const override;

    void initComplete();
    void initIncomplete();
    void set(std::vector<StructType::Field> fields, std::vector<StructType::Method> methods);
    
    StructType* get();
    lexer::Token& getToken();

    static PendingStructType* Create(lexer::Token token, std::string name, std::vector<StructType::Field> fields, std::vector<StructType::Method> methods);
    static PendingStructType* Create(lexer::Token token, std::string name, Type* base);
    static std::vector<PendingStructType*>& GetPending();

private:
    lexer::Token mToken;
    std::variant<std::monostate, StructType, IncompleteStructType, EnumType> mImpl;
    std::vector<StructType::Field> mFields;
    std::vector<StructType::Method> mMethods;

    Type* mBase;
};

#endif // VIPER_FRAMEWORK_TYPE_STRUCT_TYPE_H