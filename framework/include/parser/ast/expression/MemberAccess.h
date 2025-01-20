// Copyright 2025 solar-mist

#ifndef VIPER_FRAMEWORK_PARSER_AST_EXPRESSION_MEMBER_ACCESS_H
#define VIPER_FRAMEWORK_PARSER_AST_EXPRESSION_MEMBER_ACCESS_H 1

#include "parser/ast/ASTNode.h"

#include "lexer/Token.h"

#include <type/StructType.h>

namespace parser
{
    class MemberAccess : public ASTNode
    {
    friend class CallExpression;
    public:
        MemberAccess(ASTNodePtr structNode, std::string id, bool pointer, Scope* scope, lexer::Token operatorToken, lexer::Token fieldToken);

        virtual vipir::Value* codegen(vipir::IRBuilder& builder, vipir::Module& module, diagnostic::Diagnostics& diag) override;

        virtual void semanticCheck(diagnostic::Diagnostics& diag, bool& exit, bool statement) override;

        virtual void typeCheck(diagnostic::Diagnostics& diag, bool& exit) override;
        virtual bool triviallyImplicitCast(diagnostic::Diagnostics& diag, Type* destType) override;

    private:
        ASTNodePtr mStruct;
        std::string mId;
        bool mPointer;

        lexer::Token mOperatorToken;

        StructType* mStructType;
    };

    using MemberAccessPtr = std::unique_ptr<MemberAccess>;
}

#endif // VIPER_FRAMEWORK_PARSER_AST_EXPRESSION_MEMBER_ACCESS_H