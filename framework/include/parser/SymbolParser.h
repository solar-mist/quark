// Copyright 2024 solar-mist

#ifndef VIPER_FRAMEWORK_PARSER_SYMBOL_PARSER_H
#define VIPER_FRAMEWORK_PARSER_SYMBOL_PARSER_H 1

#include "parser/ast/ASTNode.h"

#include "parser/ast/global/Function.h"

#include "parser/ast/statement/ReturnStatement.h"
#include "parser/ast/statement/VariableDeclaration.h"
#include "parser/ast/statement/IfStatement.h"

#include "parser/ast/expression/BinaryExpression.h"
#include "parser/ast/expression/IntegerLiteral.h"
#include "parser/ast/expression/UnaryExpression.h"
#include "parser/ast/expression/VariableExpression.h"
#include "parser/ast/expression/CallExpression.h"
#include "parser/ast/expression/StringLiteral.h"

#include "lexer/Token.h"

#include "diagnostic/Diagnostic.h"

#include "symbol/Scope.h"

#include "type/Type.h"

#include <functional>

namespace parser
{
    class SymbolParser
    {
    public:
        SymbolParser(std::vector<lexer::Token>& tokens, diagnostic::Diagnostics& diag, Scope* globalScope);

        std::vector<ASTNodePtr> parse();

    private:
        std::vector<lexer::Token>& mTokens;
        unsigned int mPosition;

        diagnostic::Diagnostics& mDiag;

        Scope* mActiveScope;

        std::function<void(ASTNodePtr&)> mInsertNodeFn;

        lexer::Token current() const;
        lexer::Token consume();
        lexer::Token peek(int offset) const;

        void expectToken(lexer::TokenType tokenType);

        Type* parseType();

        ASTNodePtr parseGlobal(bool exported = false);

        FunctionPtr parseFunction(bool pure, bool exported);
        void parseImport(bool exported);
    };
}

#endif // VIPER_FRAMEWORK_PARSER_PARSER_H