// Copyright 2025 solar-mist

#ifndef VIPER_FRAMEWORK_PARSER_AST_GLOBAL_ENUM_DECLARATION_H
#define VIPER_FRAMEWORK_PARSER_AST_GLOBAL_ENUM_DECLARATION_H 1

#include "parser/ast/ASTNode.h"
#include "parser/ast/global/Function.h"

#include <memory>
#include <string>
#include <vector>

namespace parser
{
    struct EnumField
    {
        std::string name;
        unsigned long long value;

        int symbolId;
    };

    class EnumDeclaration : public ASTNode
    {
    friend struct ::ASTNodeIntrospector;
    public:
        EnumDeclaration(bool exported, bool pending, std::string name, std::vector<EnumField> fields, Type* base, ScopePtr ownScope, lexer::Token token);

        virtual std::vector<ASTNode*> getContained() const override;
        virtual ASTNodePtr clone(Scope* in) override;

        virtual vipir::Value* codegen(vipir::IRBuilder& builder, vipir::Module& module, diagnostic::Diagnostics& diag) override;

        virtual void semanticCheck(diagnostic::Diagnostics& diag, bool& exit, bool statement) override;

        virtual void typeCheck(diagnostic::Diagnostics& diag, bool& exit) override;
        virtual bool triviallyImplicitCast(diagnostic::Diagnostics& diag, Type* destType) override;

    private:
        std::string mName;
        std::vector<EnumField> mFields;
        Type* mBaseType;
        ScopePtr mOwnScope;
        unsigned long mSymbolId;
    };
    using EnumDeclarationPtr = std::unique_ptr<EnumDeclaration>;
}

#endif // VIPER_FRAMEWORK_PARSER_AST_GLOBAL_ENUM_DECLARATION_H