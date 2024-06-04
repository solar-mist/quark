// Copyright 2023 solar-mist

#ifndef VIPER_FRAMEWORK_PARSER_IMPORT_PARSER_H
#define VIPER_FRAMEWORK_PARSER_IMPORT_PARSER_H 1

#include "parser/ast/Node.h"
#include "parser/ast/global/Function.h"
#include "parser/ast/global/StructDeclaration.h"
#include "parser/ast/global/GlobalDeclaration.h"
#include "parser/ast/global/Namespace.h"
#include "parser/ast/global/UsingDeclaration.h"
#include "parser/ast/global/EnumDeclaration.h"
#include "parser/ast/statement/ConstexprStatement.h"

#include "lexer/Token.h"

#include "symbol/Import.h"

#include "diagnostic/Diagnostic.h"

#include <vector>

namespace parser
{
    class GlobalSymbol;

    class ImportParser
    {
    public:
        ImportParser(std::vector<lexing::Token>& tokens, diagnostic::Diagnostics& diag, symbol::ImportManager& importManager, bool hoistingParser = false);

        std::vector<ASTNodePtr> parse();

        std::vector<GlobalSymbol> getSymbols();

    private:
        std::vector<lexing::Token>& mTokens;
        int mPosition;

        symbol::ImportManager& mImportManager;

        Scope* mScope;
        std::vector<GlobalSymbol> mSymbols;
        std::vector<Type*> mStructTypesToRemove;

        diagnostic::Diagnostics& mDiag;

        std::vector<std::string> mNamespaces;

        bool mHoistingParser;

        lexing::Token current() const;
        lexing::Token consume();
        lexing::Token peek(int offset) const;

        void expectToken(lexing::TokenType tokenType);
        void expectEitherToken(std::vector<lexing::TokenType> tokenTypes);

        Type* parseType();

        ASTNodePtr parseGlobal(std::vector<ASTNodePtr>& nodes);
        FunctionPtr parseFunction(bool exported, std::vector<GlobalAttribute> attributes);
        NamespacePtr parseNamespace();
        StructDeclarationPtr parseStructDeclaration(bool exported);
        GlobalDeclarationPtr parseGlobalDeclaration(bool exported);
        ConstexprStatementPtr parseConstExpr(bool exported);
        std::pair<std::vector<ASTNodePtr>, std::vector<GlobalSymbol>> parseImportStatement(bool exported);
        UsingDeclarationPtr parseUsingDeclaration(bool exported);
        EnumDeclarationPtr parseEnumDeclaration(bool exported, std::vector<GlobalAttribute> attributes);

        void parseAttributes(std::vector<GlobalAttribute>& attributes);
    };
}

#endif