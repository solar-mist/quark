// Copyright 2025 solar-mist

#ifndef VIPER_FRAMEWORK_PARSER_AST_GLOBAL_CLASS_DECLARATION_H
#define VIPER_FRAMEWORK_PARSER_AST_GLOBAL_CLASS_DECLARATION_H 1

#include "parser/ast/ASTNode.h"
#include "parser/ast/global/Function.h"

#include <memory>
#include <string>
#include <vector>

namespace parser
{
    struct ClassField
    {
        ClassField(bool priv, Type* type, std::string name);
        bool priv;
        Type* type;
        std::string name;
    };

    struct ClassMethod
    {
        bool priv;
        bool pure;
        std::string name;
        FunctionType* type;
        std::vector<FunctionArgument> arguments;
        std::vector<ASTNodePtr> body;
        ScopePtr ownScope;
        lexer::Token errorToken;

        int symbolId;
    };

    class ClassDeclaration : public ASTNode
    {
    friend struct ::ASTNodeIntrospector;
    public:
        ClassDeclaration(bool exported, bool pending, std::string name, std::vector<ClassField> fields, std::vector<ClassMethod> methods, ScopePtr ownScope, lexer::Token token);

        virtual std::vector<ASTNode*> getContained() const override;
        virtual ASTNodePtr clone(Scope* in) override;
        virtual Symbol* getSymbol() override;

        virtual vipir::Value* codegen(vipir::IRBuilder& builder, vipir::Module& module, diagnostic::Diagnostics& diag) override;

        virtual void semanticCheck(diagnostic::Diagnostics& diag, bool& exit, bool statement) override;

        virtual void typeCheck(diagnostic::Diagnostics& diag, bool& exit) override;
        virtual bool triviallyImplicitCast(diagnostic::Diagnostics& diag, Type* destType) override;

    private:
        std::string mName;
        std::vector<ClassField> mFields;
        std::vector<ClassMethod> mMethods;
        ScopePtr mOwnScope;
        unsigned long mSymbolId;
    };
    using ClassDeclarationPtr = std::unique_ptr<ClassDeclaration>;
}

#endif // VIPER_FRAMEWORK_PARSER_AST_GLOBAL_CLASS_DECLARATION_H