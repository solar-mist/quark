// Copyright 2024 solar-mist

#ifndef VIPER_FRAMEWORK_PARSER_AST_GLOBAL_ENUM_DECLARATION_H
#define VIPER_FRAMEWORK_PARSER_AST_GLOBAL_ENUM_DECLARATION_H 1

#include "GlobalAttribute.h"
#include "parser/ast/Node.h"

namespace parser
{
    struct EnumField
    {
        std::string name;
        int value;
    };

    class EnumDeclaration : public ASTNode
    {
    public:
        EnumDeclaration(std::vector<GlobalAttribute> attributes, std::vector<std::string> names, std::vector<EnumField> fields);

        void typeCheck(Scope* scope, diagnostic::Diagnostics& diag) override;
        vipir::Value* emit(vipir::IRBuilder& builder, vipir::Module& module, Scope* scope, diagnostic::Diagnostics& diag) override;

    private:
        std::vector<GlobalAttribute> mAttributes;
        std::vector<std::string> mNames;
        std::vector<EnumField> mFields;
    };
    using EnumDeclarationPtr = std::unique_ptr<EnumDeclaration>;
}

#endif // VIPER_FRAMEWORK_PARSER_AST_GLOBAL_ENUM_DECLARATION_H