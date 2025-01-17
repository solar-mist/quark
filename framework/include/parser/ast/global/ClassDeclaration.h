// Copyright 2025 solar-mist

#ifndef VIPER_FRAMEWORK_PARSER_AST_GLOBAL_CLASS_DECLARATION_H
#define VIPER_FRAMEWORK_PARSER_AST_GLOBAL_CLASS_DECLARATION_H 1

#include "parser/ast/ASTNode.h"

#include <memory>
#include <string>
#include <vector>

namespace parser
{
    struct ClassField
    {
        ClassField(Type* type, std::string name);
        Type* type;
        std::string name;
    };

    class ClassDeclaration : public ASTNode
    {
    friend struct ::ASTNodeIntrospector;
    public:
        ClassDeclaration(bool exported, std::string name, std::vector<ClassField> fields, Scope* scope, lexer::Token token);

        virtual vipir::Value* codegen(vipir::IRBuilder& builder, vipir::Module& module, diagnostic::Diagnostics& diag) override;

        virtual void semanticCheck(diagnostic::Diagnostics& diag, bool& exit, bool statement) override;

        virtual void typeCheck(diagnostic::Diagnostics& diag, bool& exit) override;
        virtual bool triviallyImplicitCast(diagnostic::Diagnostics& diag, Type* destType) override;

    private:
        std::string mName;
        std::vector<ClassField> mFields;
        unsigned long mSymbolId;
    };
    using ClassDeclarationPtr = std::unique_ptr<ClassDeclaration>;
}

#endif // VIPER_FRAMEWORK_PARSER_AST_GLOBAL_CLASS_DECLARATION_H