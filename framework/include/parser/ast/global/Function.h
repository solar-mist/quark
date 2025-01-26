// Copyright 2024 solar-mist

#ifndef VIPER_FRAMEWORK_PARSER_AST_GLOBAL_FUNCTION_H
#define VIPER_FRAMEWORK_PARSER_AST_GLOBAL_FUNCTION_H 1

#include "parser/ast/ASTNode.h"

#include "type/FunctionType.h"

#include <memory>
#include <string>
#include <vector>

namespace parser
{
    struct FunctionArgument
    {
        FunctionArgument(Type* type, std::string name);
        Type* type;
        std::string name;
    };

    class Function : public ASTNode
    {
    friend struct ::ASTNodeIntrospector;
    public:
        Function(bool exported, bool pure, std::string name, FunctionType* type, std::vector<FunctionArgument> arguments, std::vector<ASTNodePtr> body, ScopePtr scope, lexer::Token token);

        virtual void setTemplateType(Type* templateType, Type* type) override;
        virtual std::vector<ASTNode*> getContained() const override;
        virtual ASTNodePtr clone(Scope* in) override;
        virtual Symbol* getSymbol() override;

        virtual vipir::Value* codegen(vipir::IRBuilder& builder, vipir::Module& module, diagnostic::Diagnostics& diag) override;

        virtual void semanticCheck(diagnostic::Diagnostics& diag, bool& exit, bool statement) override;

        virtual void typeCheck(diagnostic::Diagnostics& diag, bool& exit) override;
        virtual bool triviallyImplicitCast(diagnostic::Diagnostics& diag, Type* destType) override;

    private:
        bool mPure;
        std::string mName;
        std::vector<FunctionArgument> mArguments;
        std::vector<ASTNodePtr> mBody;
        unsigned long mSymbolId;

        ScopePtr mOwnScope;
    };
    using FunctionPtr = std::unique_ptr<Function>;
}

#endif // VIPER_FRAMEWORK_PARSER_AST_GLOBAL_FUNCTION_H