// Copyright 2024 solar-mist

#ifndef VIPER_FRAMEWORK_TYPE_TYPE_H
#define VIPER_FRAMEWORK_TYPE_TYPE_H 1

#include <vipir/Type/Type.h>

#include <memory>


class Type
{
public:
    enum class CastLevel
    {
        Implicit,
        ImplicitWarning,
        Explicit,
        Disallowed
    };

    Type(std::string name) : mName(std::move(name)) { }
    virtual ~Type() {}

    virtual int getSize() const = 0;
    virtual vipir::Type* getVipirType() const = 0;
    virtual CastLevel castTo(Type* destType) const = 0;
    virtual std::string getMangleId() const = 0;
    virtual std::string getImplicitCastWarning(Type* destType) const { return ""; }
    virtual Type* replaceWith(Type* from, Type* to) { return this; }

    virtual bool isIntegerType()  const { return false; }
    virtual bool isVoidType()     const { return false; }
    virtual bool isFunctionType() const { return false; }
    virtual bool isBooleanType()  const { return false; }
    virtual bool isPointerType()  const { return false; }
    virtual bool isStructType()   const { return false; }
    virtual bool isEnumType()     const { return false; }
    virtual bool isObjectType()   const { return true; }
    virtual bool isTemplateType() const { return false; }

    static void Init();
    static bool Exists(const std::string& name);
    static Type* Get(const std::string& name);

    virtual std::string_view getName() { return mName; }

protected:
    std::string mName;
};

#endif // VIPER_FRAMEWORK_TYPE_TYPE_H