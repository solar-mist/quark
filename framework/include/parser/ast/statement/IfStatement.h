// Copyright 2024 solar-mist

#ifndef VIPER_FRAMEWORK_PARSER_AST_STATEMENT_IF_STATEMENT_H
#define VIPER_FRAMEWORK_PARSER_AST_STATEMENT_IF_STATEMENT_H 1

#include "parser/ast/ASTNode.h"

#include <memory>

namespace parser
{
    class IfStatement : public ASTNode
    {
    friend class ::ASTNodeIntrospector;
    public:
        IfStatement(ASTNodePtr&& condition, ASTNodePtr&& body, ASTNodePtr&& elseBody, ScopePtr scope, lexer::Token token);

        virtual std::vector<ASTNode*> getContained() const override;
        virtual ASTNodePtr clone(Scope* in) override;

        virtual vipir::Value* codegen(vipir::IRBuilder& builder, vipir::Module& module, diagnostic::Diagnostics& diag) override;

        virtual void semanticCheck(diagnostic::Diagnostics& diag, bool& exit, bool statement) override;

        virtual void typeCheck(diagnostic::Diagnostics& diag, bool& exit) override;
        virtual bool triviallyImplicitCast(diagnostic::Diagnostics& diag, Type* destType) override;

    private:
        ASTNodePtr mCondition;
        ASTNodePtr mBody;
        ASTNodePtr mElseBody;

        ScopePtr mOwnScope;
    };
    using IfStatementPtr = std::unique_ptr<IfStatement>;
}

#endif // VIPER_FRAMEWORK_PARSER_AST_STATEMENT_IF_STATEMENT_H