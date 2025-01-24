// Copyright 2024 solar-mist

#include "parser/SymbolParser.h"

#include "type/PointerType.h"
#include "type/StructType.h"

#include <cinttypes>
#include <filesystem>
#include <format>
#include <numeric>

namespace parser
{
    SymbolParser::SymbolParser(std::vector<lexer::Token>& tokens, diagnostic::Diagnostics& diag, ImportManager& importManager, Scope* globalScope)
        : mTokens(tokens)
        , mPosition(0)
        , mDiag(diag)
        , mActiveScope(globalScope)
        , mExportBlock(false)
        , mImportManager(importManager)
    {
    }

    std::vector<std::filesystem::path> SymbolParser::findImports()
    {
        std::vector<std::filesystem::path> ret;

        while (current().getTokenType() == lexer::TokenType::ImportKeyword)
        {
            consume();
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
            ret.push_back(std::move(path));
        }

        return ret;
    }

    std::vector<ASTNodePtr> SymbolParser::parse()
    {
        std::vector<ASTNodePtr> ast;

        mInsertNodeFn = [&ast](ASTNodePtr& node) {
            if (node) ast.push_back(std::move(node));
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

        Type* type = nullptr;
        if (current().getTokenType() == lexer::TokenType::Identifier)
        {
            auto var = parseVariableExpression();
            auto names = var->getNames();
            auto mangled = StructType::MangleName(names);
            
            if (auto structType = StructType::Get(mangled))
            {
                type = structType;
            }
            // In case of incomplete struct type from imported file
            if (auto structType = Type::Get(mangled))
            {
                type = structType;
            }

            if (!type)
            {
                // Create an empty pending struct type for now which will be resolved later
                auto token = peek(-1);
                auto mangled = StructType::MangleName(names);
                type = PendingStructType::Create(std::move(token), std::move(mangled), {});
            }
        }
        if (!type) // No struct type was found
        {
            expectToken(lexer::TokenType::TypeKeyword);
            type = Type::Get(std::string(consume().getText()));
        }

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
                if (current().getTokenType() == lexer::TokenType::LeftBrace)
                {
                    consume();
                    mExportBlock = true;
                    while (current().getTokenType() != lexer::TokenType::RightBrace)
                    {
                        auto node = parseGlobal(true);
                        mInsertNodeFn(node);
                    }
                    consume();
                    return nullptr;
                }
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

            case lexer::TokenType::ClassKeyword:
                return parseClassDeclaration(exported);

            case lexer::TokenType::NamespaceKeyword:
                return parseNamespace(exported);

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

    ClassDeclarationPtr SymbolParser::parseClassDeclaration(bool exported)
    {
        auto token = consume(); // class

        expectToken(lexer::TokenType::Identifier);
        std::string name = std::string(consume().getText());

        expectToken(lexer::TokenType::LeftBrace);
        consume();

        std::vector<ClassField> fields;
        while (current().getTokenType() != lexer::TokenType::RightBrace)
        {
            expectToken(lexer::TokenType::Identifier);
            std::string fieldName = std::string(consume().getText());

            expectToken(lexer::TokenType::Colon);
            consume();

            Type* fieldType = parseType();
            fields.push_back(ClassField(fieldType, std::move(fieldName)));

            if (current().getTokenType() != lexer::TokenType::RightBrace)
            {
                expectToken(lexer::TokenType::Semicolon);
                consume();
            }
        }
        consume();

        auto classDeclaration = std::make_unique<ClassDeclaration>(exported, true, name, std::move(fields), mActiveScope, std::move(token));
        std::vector<std::string> names = mActiveScope->getNamespaces();
        names.push_back(name);
        mImportManager.addPendingStructType(std::move(names));
        return classDeclaration;
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
        return;

        ScopePtr scope = std::make_unique<Scope>(nullptr, "", true);

        auto nodes = mImportManager.resolveImports(path, mTokens[0].getStartLocation().file, scope.get(), exported);
        for (auto& node : nodes)
        {
            mInsertNodeFn(node);
        }
        mActiveScope->children.push_back(scope.get());
        mImportManager.seizeScope(std::move(scope));
    }

    NamespacePtr SymbolParser::parseNamespace(bool exported)
    {
        consume(); // namespace

        auto token = consume();
        std::string name = std::string(token.getText());

        expectToken(lexer::TokenType::LeftBrace);
        consume();

        ScopePtr scope = std::make_unique<Scope>(mActiveScope, name, true);
        mActiveScope = scope.get();
        std::vector<ASTNodePtr> body;
        while (current().getTokenType() != lexer::TokenType::RightBrace)
        {
            auto node = parseGlobal(exported || mExportBlock);
            if (node) body.push_back(std::move(node));
        }
        consume();
        mActiveScope = scope->parent;

        return std::make_unique<Namespace>(exported, std::move(name), std::move(body), std::move(scope), std::move(token));
    }

    VariableExpressionPtr SymbolParser::parseVariableExpression()
    {
        auto token = consume();
        std::vector<std::string> names;
        names.push_back(std::string(token.getText()));

        while (current().getTokenType() == lexer::TokenType::DoubleColon)
        {
            consume();
            expectToken(lexer::TokenType::Identifier);
            token = consume();
            names.push_back(std::string(token.getText()));
        }

        if (names.size() == 1)
            return std::make_unique<VariableExpression>(mActiveScope, std::move(names[0]), std::move(token));

        return std::make_unique<VariableExpression>(mActiveScope, std::move(names), std::move(token));
    }
}