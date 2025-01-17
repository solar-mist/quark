// Copyright 2024 solar-mist

#include "parser/SymbolParser.h"

#include "symbol/ImportManager.h"
#include "type/PointerType.h"

#include <cinttypes>
#include <filesystem>
#include <format>

namespace parser
{
    SymbolParser::SymbolParser(std::vector<lexer::Token>& tokens, diagnostic::Diagnostics& diag, Scope* globalScope)
        : mTokens(tokens)
        , mPosition(0)
        , mDiag(diag)
        , mActiveScope(globalScope)
    {
    }

    std::vector<ASTNodePtr> SymbolParser::parse()
    {
        std::vector<ASTNodePtr> ast;

        mInsertNodeFn = [&ast](ASTNodePtr& node) {
            ast.push_back(std::move(node));
        };

        while (mPosition < mTokens.size())
        {
            auto global = parseGlobal();
            if (global)
            {
                ast.push_back(std::move(global));
            }
        }

        return ast;
    }

    lexer::Token SymbolParser::current() const
    {
        return mTokens[mPosition];
    }

    lexer::Token SymbolParser::consume()
    {
        return mTokens[mPosition++];
    }

    lexer::Token SymbolParser::peek(int offset) const
    {
        return mTokens[mPosition + offset];
    }

    void SymbolParser::expectToken(lexer::TokenType tokenType)
    {
        if (current().getTokenType() != tokenType)
        {
            lexer::Token temp("", tokenType, lexer::SourceLocation(), lexer::SourceLocation());
            mDiag.reportCompilerError(
                current().getStartLocation(),
                current().getEndLocation(),
                std::format("Expected '{}{}{}', found '{}{}{}'",
                    fmt::bold, temp.getName(), fmt::defaults,
                    fmt::bold, current().getText(), fmt::defaults)
            );
            std::exit(1);
        }
    }

    Type* SymbolParser::parseType()
    {
        if (current().getTokenType() == lexer::TokenType::LeftParen) // function pointer
        {
            consume();
            std::vector<Type*> argumentTypes;
            while (current().getTokenType() != lexer::TokenType::RightParen)
            {
                argumentTypes.push_back(parseType());
                if (current().getTokenType() != lexer::TokenType::RightParen)
                {
                    expectToken(lexer::TokenType::Comma);
                    consume();
                }
            }
            consume();

            int pointerLevels = 0;
            expectToken(lexer::TokenType::Star);
            while (current().getTokenType() == lexer::TokenType::Star)
            {
                ++pointerLevels;
                consume();
            }

            expectToken(lexer::TokenType::RightArrow);
            consume();

            Type* returnType = parseType();
            Type* type = FunctionType::Create(returnType, std::move(argumentTypes));
            while(pointerLevels--)
            {
                type = PointerType::Get(type);
            }
            return type;
        }

        expectToken(lexer::TokenType::TypeKeyword);
        auto type = Type::Get(std::string(consume().getText()));

        while (current().getTokenType() == lexer::TokenType::Star)
        {
            consume();
            type = PointerType::Get(type);
        }

        return type;
    }


    ASTNodePtr SymbolParser::parseGlobal(bool exported)
    {
        switch (current().getTokenType())
        {
            case lexer::TokenType::ExportKeyword:
                consume();
                return parseGlobal(true);

            case lexer::TokenType::ImportKeyword:
                parseImport(exported);
                return nullptr;

            case lexer::TokenType::PureKeyword:
                consume();
                expectToken(lexer::TokenType::FuncKeyword);
                return parseFunction(true, exported);
            case lexer::TokenType::FuncKeyword:
                return parseFunction(false, exported);

            case lexer::TokenType::EndOfFile:
                consume();
                return nullptr;

            default:
                mDiag.reportCompilerError(
                    current().getStartLocation(),
                    current().getEndLocation(),
                    std::format("Expected global expression. Found '{}{}{}'", fmt::bold, current().getText(), fmt::defaults)
                );
                std::exit(1);
                return nullptr;
        }
    }


    FunctionPtr SymbolParser::parseFunction(bool pure, bool exported)
    {
        auto token = consume(); // FuncKeyword

        expectToken(lexer::TokenType::Identifier);
        std::string name = std::string(consume().getText());

        std::vector<FunctionArgument> arguments;
        std::vector<Type*> argumentTypes;
        expectToken(lexer::TokenType::LeftParen);
        consume();
        while (current().getTokenType() != lexer::TokenType::RightParen)
        {
            expectToken(lexer::TokenType::Identifier);
            auto name = std::string(consume().getText());
            
            expectToken(lexer::TokenType::Colon);
            consume();

            auto type = parseType();
            arguments.emplace_back(type, std::move(name));
            argumentTypes.push_back(type);
        }
        consume();

        expectToken(lexer::TokenType::RightArrow);
        consume();
        Type* returnType = parseType();

        FunctionType* functionType = FunctionType::Create(returnType, std::move(argumentTypes));

        ScopePtr scope = std::make_unique<Scope>(mActiveScope, "", false, returnType);
        mActiveScope = scope.get();

        if (current().getTokenType() == lexer::TokenType::Semicolon)
        {
            consume();
            mActiveScope = scope->parent;
            return std::make_unique<Function>(exported, pure, std::move(name), functionType, std::move(arguments), std::vector<ASTNodePtr>(), std::move(scope), std::move(token));
        }

        std::vector<ASTNodePtr> body;
        expectToken(lexer::TokenType::LeftBrace);
        consume();

        while (current().getTokenType() != lexer::TokenType::RightBrace)
        {
            consume();
        }
        consume();

        mActiveScope = scope->parent;

        return std::make_unique<Function>(exported, pure, std::move(name), functionType, std::move(arguments), std::move(body), std::move(scope), std::move(token));
    }

    void SymbolParser::parseImport(bool exported)
    {
        consume(); // ImportKeyword

        std::filesystem::path path;
        while (current().getTokenType() != lexer::TokenType::Semicolon)
        {
            expectToken(lexer::TokenType::Identifier);
            path /= consume().getText();

            if (current().getTokenType() != lexer::TokenType::Semicolon)
            {
                expectToken(lexer::TokenType::Dot);
                consume();
            }
        }
        consume();
        if (!exported) return;

        ScopePtr scope = std::make_unique<Scope>(nullptr, "", true);

        ImportManager importManager;
        auto nodes = importManager.resolveImports(path, scope.get());
        for (auto& node : nodes)
        {
            mInsertNodeFn(node);
        }
        mActiveScope->importedScopes.push_back(std::move(scope));
    }
}