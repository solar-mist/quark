// Copyright 2024 solar-mist

#ifndef VIPER_FRAMEWORK_PARSER_PARSER_H
#define VIPER_FRAMEWORK_PARSER_PARSER_H 1

#include "parser/ast/ASTNode.h"

#include "parser/ast/global/Function.h"
#include "parser/ast/global/ClassDeclaration.h"
#include "parser/ast/global/Namespace.h"
#include "parser/ast/global/EnumDeclaration.h"

#include "parser/ast/statement/ReturnStatement.h"
#include "parser/ast/statement/VariableDeclaration.h"
#include "parser/ast/statement/IfStatement.h"

#include "parser/ast/expression/BinaryExpression.h"
#include "parser/ast/expression/IntegerLiteral.h"
#include "parser/ast/expression/UnaryExpression.h"
#include "parser/ast/expression/VariableExpression.h"
#include "parser/ast/expression/CallExpression.h"
#include "parser/ast/expression/StringLiteral.h"
#include "parser/ast/expression/MemberAccess.h"
#include "parser/ast/expression/CastExpression.h"

#include "lexer/Token.h"

#include "diagnostic/Diagnostic.h"

#include "symbol/Scope.h"
#include "symbol/ImportManager.h"

#include "type/Type.h"

#include <functional>

namespace parser
{
    class Parser
    {
    friend class ::ASTNodeIntrospector;
    public:
        Parser(std::vector<lexer::Token>& tokens, diagnostic::Diagnostics& diag, ImportManager& importManager, Scope* globalScope, bool isImporter = false);

        std::vector<std::filesystem::path> findImports();

        std::vector<ASTNodePtr> parse();
        std::vector<TemplateSymbol*> getTemplatedSymbols();

    private:
        std::vector<lexer::Token>& mTokens;
        unsigned int mPosition;

        diagnostic::Diagnostics& mDiag;

        Scope* mActiveScope;

        bool mExportBlock;
        ImportManager& mImportManager;
        bool mIsImporter;

        std::vector<TemplateParameter> mActiveTemplateParameters;
        std::vector<TemplateSymbol*> mTemplateSymbols;
        std::function<void(ASTNodePtr&)> mInsertNodeFn;


        lexer::Token current() const;
        lexer::Token consume();
        lexer::Token peek(int offset) const;

        void expectToken(lexer::TokenType tokenType);

        int getBinaryOperatorPrecedence(lexer::TokenType tokenType);
        int getPrefixUnaryOperatorPrecedence(lexer::TokenType tokenType);
        int getPostfixUnaryOperatorPrecedence(lexer::TokenType tokenType);

        Type* parseType();

        ASTNodePtr parseGlobal(bool exported = false);
        ASTNodePtr parseExpression(int precedence = 1);
        ASTNodePtr parsePrimary();

        FunctionPtr parseFunction(bool pure, bool exported);
        ClassDeclarationPtr parseClassDeclaration(bool exported);
        ClassMethod parseClassMethod(bool priv, bool pure);
        NamespacePtr parseNamespace(bool exported);
        EnumDeclarationPtr parseEnum(bool exported);
        void parseTemplate(bool exported);
        void parseImport();

        ReturnStatementPtr parseReturnStatement();
        VariableDeclarationPtr parseVariableDeclaration();
        IfStatementPtr parseIfStatement();

        IntegerLiteralPtr parseIntegerLiteral();
        VariableExpressionPtr parseVariableExpression();
        CallExpressionPtr parseCallExpression(ASTNodePtr callee);
        StringLiteralPtr parseStringLiteral();
        MemberAccessPtr parseMemberAccess(ASTNodePtr structNode, bool pointer);
        CastExpressionPtr parseCastExpression();
    };
}

#endif // VIPER_FRAMEWORK_PARSER_PARSER_H