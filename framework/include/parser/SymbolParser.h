// Copyright 2024 solar-mist

#ifndef VIPER_FRAMEWORK_PARSER_SYMBOL_PARSER_H
#define VIPER_FRAMEWORK_PARSER_SYMBOL_PARSER_H 1

#include "parser/ast/ASTNode.h"

#include "parser/ast/global/Function.h"
#include "parser/ast/global/ClassDeclaration.h"

#include "lexer/Token.h"

#include "diagnostic/Diagnostic.h"

#include "symbol/Scope.h"
#include "symbol/ImportManager.h"

#include "type/Type.h"

#include <functional>

namespace parser
{
    class SymbolParser
    {
    public:
        SymbolParser(std::vector<lexer::Token>& tokens, diagnostic::Diagnostics& diag, ImportManager& importManager, Scope* globalScope);

        std::vector<std::filesystem::path> findImports();

        std::vector<ASTNodePtr> parse();

    private:
        std::vector<lexer::Token>& mTokens;
        unsigned int mPosition;

        diagnostic::Diagnostics& mDiag;

        Scope* mActiveScope;

        bool mExportBlock;
        ImportManager& mImportManager;

        std::function<void(ASTNodePtr&)> mInsertNodeFn;

        lexer::Token current() const;
        lexer::Token consume();
        lexer::Token peek(int offset) const;

        void expectToken(lexer::TokenType tokenType);

        Type* parseType();

        ASTNodePtr parseGlobal(bool exported = false);

        FunctionPtr parseFunction(bool pure, bool exported);
        ClassDeclarationPtr parseClassDeclaration(bool exported);
        void parseImport(bool exported);
    };
}

#endif // VIPER_FRAMEWORK_PARSER_PARSER_H