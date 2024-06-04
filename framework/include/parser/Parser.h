// Copyright 2023 solar-mist

#ifndef VIPER_FRAMEWORK_PARSER_PARSER_H
#define VIPER_FRAMEWORK_PARSER_PARSER_H 1

#include "parser/ast/Node.h"
#include "parser/ast/global/Function.h"
#include "parser/ast/global/StructDeclaration.h"
#include "parser/ast/global/GlobalDeclaration.h"
#include "parser/ast/global/Namespace.h"
#include "parser/ast/global/UsingDeclaration.h"
#include "parser/ast/global/EnumDeclaration.h"

#include "parser/ast/statement/ReturnStatement.h"
#include "parser/ast/statement/VariableDeclaration.h"
#include "parser/ast/statement/IfStatement.h"
#include "parser/ast/statement/WhileStatement.h"
#include "parser/ast/statement/CompoundStatement.h"
#include "parser/ast/statement/ForStatement.h"
#include "parser/ast/statement/SwitchStatement.h"
#include "parser/ast/statement/ConstexprStatement.h"

#include "parser/ast/expression/IntegerLiteral.h"
#include "parser/ast/expression/StringLiteral.h"
#include "parser/ast/expression/VariableExpression.h"
#include "parser/ast/expression/CallExpression.h"
#include "parser/ast/expression/MemberAccess.h"
#include "parser/ast/expression/StructInitializer.h"
#include "parser/ast/expression/ArrayInitializer.h"
#include "parser/ast/expression/SizeofExpression.h"

#include "lexer/Token.h"

#include "symbol/Import.h"

#include "diagnostic/Diagnostic.h"

#include <vector>

namespace parser
{
    struct GlobalSymbol
    {
        std::string name;
        Type* type;
    };

    class Parser
    {
    public:
        Parser(std::vector<lexing::Token>& tokens, diagnostic::Diagnostics& diag, symbol::ImportManager& importManager);

        std::vector<ASTNodePtr> parse();

    private:
        std::vector<lexing::Token>& mTokens;
        int mPosition;

        symbol::ImportManager& mImportManager;

        Scope* mScope;
        std::vector<GlobalSymbol> mSymbols;

        diagnostic::Diagnostics& mDiag;

        std::vector<std::string> mNamespaces;

        lexing::Token current() const;
        lexing::Token consume();
        lexing::Token peek(int offset) const;

        void expectToken(lexing::TokenType tokenType);
        void expectEitherToken(std::vector<lexing::TokenType> tokenTypes);

        int getBinaryOperatorPrecedence(lexing::TokenType tokenType);
        int getPrefixUnaryOperatorPrecedence(lexing::TokenType tokenType);
        int getPostfixUnaryOperatorPrecedence(lexing::TokenType tokenType);

        Type* parseType(bool failable = false);

        ASTNodePtr parseGlobal(std::vector<ASTNodePtr>& nodes);
        ASTNodePtr parseExpression(Type* preferredType = nullptr, int precedence = 1);
        ASTNodePtr parsePrimary(Type* preferredType = nullptr);
        ASTNodePtr parseParenthesizedExpression(Type* preferredType = nullptr);

        FunctionPtr parseFunction(std::vector<GlobalAttribute> attributes);
        NamespacePtr parseNamespace();
        StructDeclarationPtr parseStructDeclaration();
        GlobalDeclarationPtr parseGlobalDeclaration();
        std::pair<std::vector<ASTNodePtr>, std::vector<GlobalSymbol>> parseImportStatement();
        UsingDeclarationPtr parseUsingDeclaration();
        EnumDeclarationPtr parseEnumDeclaration(std::vector<GlobalAttribute> attributes);

        CompoundStatementPtr parseCompoundStatement();
        ReturnStatementPtr parseReturnStatement();
        VariableDeclarationPtr parseVariableDeclaration();
        ConstexprStatementPtr parseConstexprStatement(bool global);
        IfStatementPtr parseIfStatement();
        WhileStatementPtr parseWhileStatement();
        ForStatementPtr parseForStatement();
        SwitchStatementPtr parseSwitchStatement();

        SizeofExpressionPtr parseSizeof(Type* preferredType = nullptr);
        IntegerLiteralPtr parseIntegerLiteral(Type* preferredType = nullptr);
        StringLiteralPtr parseStringLiteral();
        VariableExpressionPtr parseVariableExpression(Type* preferredType = nullptr);
        CallExpressionPtr parseCallExpression(ASTNodePtr function);
        MemberAccessPtr parseMemberAccess(ASTNodePtr struc, bool pointer);
        StructInitializerPtr parseStructInitializer(Type* type, lexing::Token token);
        ArrayInitializerPtr parseArrayInitializer(Type* preferredType = nullptr);

        void parseAttributes(std::vector<GlobalAttribute>& attributes);
    };
}

#endif