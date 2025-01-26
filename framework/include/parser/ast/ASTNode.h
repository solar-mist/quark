// Copyright 2024 solar-mist

#ifndef VIPER_FRAMEWORK_PARSER_AST_AST_NODE_H
#define VIPER_FRAMEWORK_PARSER_AST_AST_NODE_H 1

#include "symbol/Scope.h"

#include "type/Type.h"

#include "lexer/Token.h"

#include "diagnostic/Diagnostic.h"

#include <vipir/IR/IRBuilder.h>

#include <cassert>
#include <memory>

struct ASTNodeIntrospector;

namespace parser
{
    class ASTNode
    {
    friend struct ::ASTNodeIntrospector;
    public:
        using ASTNodePtr = std::unique_ptr<ASTNode>;

        ASTNode(Scope* scope, lexer::Token errorToken) : mScope(scope), mErrorToken(errorToken) { }
        ASTNode(Scope* scope, Type* type, lexer::Token errorToken) : mScope(scope), mType(type), mErrorToken(errorToken) { }
        virtual ~ASTNode() { }

        Scope* getScope() const { return mScope; }
        Type* getType() const { return mType; }
        virtual void setTemplateType(Type* templateType, Type* type) { if (mType) { if (mType == templateType) mType = type; mType->replaceWith(templateType, type); } }
        const lexer::Token& getErrorToken() const { return mErrorToken; }

        virtual std::vector<ASTNode*> getContained() const = 0;
        virtual ASTNodePtr clone(Scope* in) = 0;
        virtual Symbol* getSymbol() { return nullptr; }

        virtual vipir::Value* codegen(vipir::IRBuilder& builder, vipir::Module& module, diagnostic::Diagnostics& diag) = 0;

        virtual void semanticCheck(diagnostic::Diagnostics& diag, bool& exit, bool statement) = 0;

        virtual void typeCheck(diagnostic::Diagnostics& diag, bool& exit) = 0;
        virtual bool triviallyImplicitCast(diagnostic::Diagnostics& diag, Type* destType) = 0;
        bool implicitCast(diagnostic::Diagnostics& diag, Type* destType);

        static ASTNodePtr Cast(ASTNodePtr& node, Type* destType);

    protected:
        Scope* mScope;
        Type* mType{nullptr};

        lexer::Token mErrorToken;
    };
    using ASTNodePtr = std::unique_ptr<ASTNode>;

}

#endif // VIPER_FRAMEWORK_PARSER_AST_AST_NODE_H