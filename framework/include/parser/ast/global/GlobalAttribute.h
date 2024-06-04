// Copyright 2024 solar-mist

#ifndef VIPER_FRAMEWORK_PARSER_AST_GLOBAL_GLOBAL_ATTRIBUTE_H
#define VIPER_FRAMEWORK_PARSER_AST_GLOBAL_GLOBAL_ATTRIBUTE_H 1

namespace parser
{
    enum class GlobalAttributeType
    {
        NoMangle,
        GenerateNames
    };

    class GlobalAttribute
    {
    public:
        GlobalAttribute(GlobalAttributeType type);

        GlobalAttributeType getType() const;

    private:
        GlobalAttributeType mType;
    };
}

#endif // VIPER_FRAMEWORK_PARSER_AST_GLOBAL_GLOBAL_ATTRIBUTE_H