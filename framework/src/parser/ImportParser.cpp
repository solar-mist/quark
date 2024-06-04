// Copyright 2023 solar-mist


#include "parser/ImportParser.h"
#include "parser/Parser.h"

#include "lexer/Token.h"

#include "symbol/Identifier.h"
#include "symbol/Import.h"

#include "type/PointerType.h"
#include "type/StructType.h"
#include "type/ArrayType.h"

#include <algorithm>
#include <filesystem>
#include <format>

namespace parser
{
    ImportParser::ImportParser(std::vector<lexing::Token>& tokens, diagnostic::Diagnostics& diag, symbol::ImportManager& importManager, bool hoistingParser)
        : mTokens(tokens)
        , mImportManager(importManager)
        , mPosition(0)
        , mScope(nullptr)
        , mDiag(diag)
        , mHoistingParser(hoistingParser)
    {
    }

    lexing::Token ImportParser::current() const
    {
        return mTokens.at(mPosition);
    }

    lexing::Token ImportParser::consume()
    {
        return mTokens.at(mPosition++);
    }

    lexing::Token ImportParser::peek(int offset) const
    {
        return mTokens.at(mPosition + offset);
    }

    void ImportParser::expectToken(lexing::TokenType tokenType)
    {
        if (current().getTokenType() != tokenType)
        {
            lexing::Token temp(tokenType, {0, 0}, {0, 0});
            mDiag.compilerError(current().getStart(), current().getEnd(), std::format("expected '{}{}{}' before '{}{}{}' token",
                fmt::bold, temp.getId(), fmt::defaults, fmt::bold, current().getId(), fmt::defaults));
        }
    }

    void ImportParser::expectEitherToken(std::vector<lexing::TokenType> tokenTypes)
    {
        std::string tokensString;
        for (auto& tokenType : tokenTypes)
        {
            if (current().getTokenType() == tokenType)
                return;

            lexing::Token temp(tokenType, {0, 0}, {0, 0});
            tokensString += std::format("'{}{}{}', ", fmt::bold, temp.getId(), fmt::defaults);
        }

        tokensString = tokensString.substr(0, tokensString.size() - 2);

        mDiag.compilerError(current().getStart(), current().getEnd(), std::format("expected either {} before '{}{}{}' token",
            tokensString, fmt::bold, current().getId(), fmt::defaults));
    }

    std::vector<ASTNodePtr> ImportParser::parse()
    {
        std::vector<ASTNodePtr> result;

        while (mPosition < mTokens.size())
        {
            auto node = parseGlobal(result);
            if (node)
            {
                result.push_back(std::move(node));
            }
        }
        for (auto type : mStructTypesToRemove)
        {
            StructType::Erase(type);
        }

        return result;
    }

    std::vector<GlobalSymbol> ImportParser::getSymbols()
    {
        return mSymbols;
    }

    ASTNodePtr ImportParser::parseGlobal(std::vector<ASTNodePtr>& nodes)
    {
        std::vector<GlobalAttribute> attributes;
        if (current().getTokenType() == lexing::TokenType::DoubleLeftSquareBracket)
        {
            parseAttributes(attributes);
        }

        bool exported = mHoistingParser;
        if (current().getTokenType() == lexing::TokenType::ExportKeyword)
        {
            exported = true;
            consume();
        }

        switch (current().getTokenType())
        {
            case lexing::TokenType::FuncKeyword:
                return parseFunction(exported, attributes);
            case lexing::TokenType::StructKeyword:
                return parseStructDeclaration(exported);
            case lexing::TokenType::GlobalKeyword:
                return parseGlobalDeclaration(exported);
            case lexing::TokenType::ConstexprKeyword:
                return parseConstExpr(exported);
            case lexing::TokenType::ImportKeyword:
            {
                auto symbols = parseImportStatement(exported);
                std::move(symbols.first.begin(), symbols.first.end(), std::back_inserter(nodes));
                std::move(symbols.second.begin(), symbols.second.end(), std::back_inserter(mSymbols));
                return nullptr;
            }
            case lexing::TokenType::NamespaceKeyword:
                return parseNamespace();
            case lexing::TokenType::UsingKeyword:
                if (peek(1).getTokenType() == lexing::TokenType::StructKeyword)
                {
                    consume();
                    StructDeclarationPtr structDecl = parseStructDeclaration(exported);
                    if (exported)
                        Type::AddAlias(structDecl->getNames(), structDecl->getType());
                    return structDecl;
                }
                return parseUsingDeclaration(exported);

            case lexing::TokenType::EnumKeyword:
                return parseEnumDeclaration(exported, attributes);
            default:
                mDiag.compilerError(current().getStart(), current().getEnd(), "Unexpected token. Expected global statement");
        }
    }

    Type* ImportParser::parseType()
    {
        Type* type = nullptr;
        if (current().getTokenType() == lexing::TokenType::StructKeyword)
        {
            consume();
            std::vector<std::string> names;
            expectToken(lexing::TokenType::Identifier);
            while (current().getTokenType() == lexing::TokenType::Identifier)
            {
                names.push_back(consume().getText());
                if (peek(1).getTokenType() == lexing::TokenType::Identifier)
                {
                    expectToken(lexing::TokenType::DoubleColon);
                    consume();
                }
            }
            std::vector<std::string> types = symbol::GetSymbol(names, mNamespaces);
            for (auto& name : types)
            {
                type = StructType::Get(name);
                if (type) break;
            }
            if (!type)
            {
                mDiag.compilerError(peek(-1).getStart(), peek(-1).getEnd(), std::format("unknown type name '{}{}{}'", fmt::bold, names.back(), fmt::defaults));
            }
        }
        else
        {
            std::vector<std::string> names;
            if (current().getTokenType() == lexing::TokenType::Type)
            {
                names.push_back(consume().getText());
            }
            else
            {
                while (current().getTokenType() == lexing::TokenType::Identifier)
                {
                    names.push_back(consume().getText());
                    if (peek(1).getTokenType() == lexing::TokenType::Identifier)
                    {
                        expectToken(lexing::TokenType::DoubleColon);
                        consume();
                    }
                }
            }

            std::vector<std::string> types = symbol::GetSymbol(names, mNamespaces);

            lexing::Token token = peek(-1);
            for (auto& name : types)
            {
                type = Type::Get(name);
                if (type) break;
            }
            if (!type)
            {
                type = Type::Get(names.front());

                if (!type)
                    mDiag.compilerError(token.getStart(), token.getEnd(), std::format("unknown type name '{}{}{}'", fmt::bold, token.getText(), fmt::defaults));
            }
        }

        while(current().getTokenType() == lexing::TokenType::Star || current().getTokenType() == lexing::TokenType::LeftSquareBracket)
        {
            if (current().getTokenType() == lexing::TokenType::Star)
            {
                consume();
                type = PointerType::Create(type);
            }
            else
            {
                consume();
                expectToken(lexing::TokenType::IntegerLiteral);
                int count = std::stoi(consume().getText(), 0, 0);
                expectToken(lexing::TokenType::RightSquareBracket);
                consume();
                type = ArrayType::Create(type, count);
            }
        }

        return type;
    }

    FunctionPtr ImportParser::parseFunction(bool exported, std::vector<GlobalAttribute> attributes)
    {
        consume();

        expectToken(lexing::TokenType::Asperand);
        consume();

        expectToken(lexing::TokenType::Identifier);
        std::string name = consume().getText();

        expectToken(lexing::TokenType::LeftParen);
        consume();

        std::vector<FunctionArgument> arguments;

        while (current().getTokenType() != lexing::TokenType::RightParen)
        {
            expectToken(lexing::TokenType::Identifier);
            std::string name = consume().getText();

            expectToken(lexing::TokenType::Colon);
            consume();

            Type* type = parseType();
            arguments.push_back({std::move(name), type});

            if (current().getTokenType() != lexing::TokenType::RightParen)
            {
                expectToken(lexing::TokenType::Comma);
                consume();
            }
        }
        consume();

        Type* returnType = Type::Get("void");
        if (current().getTokenType() == lexing::TokenType::RightArrow)
        {
            consume();
            returnType = parseType();
        }

        std::vector<Type*> argumentTypes;
        for (auto& argument : arguments)
        {
            argumentTypes.push_back(argument.type);
        }
        Type* type = FunctionType::Create(returnType, std::move(argumentTypes));

        if (current().getTokenType() == lexing::TokenType::Semicolon) // Extern function declaration
        {
            consume();
            if (exported)
                return std::make_unique<Function>(std::move(attributes), type, std::move(arguments), std::move(name), std::vector<ASTNodePtr>(), nullptr);
            return nullptr;
        }

        expectEitherToken({lexing::TokenType::LeftBracket, lexing::TokenType::Equals});
        bool isExpressionBodied = current().getTokenType() == lexing::TokenType::Equals;
        consume();

        if (isExpressionBodied)
        {
            while (current().getTokenType() != lexing::TokenType::Semicolon)
                consume();
            consume();
        }
        else
        {
            int bracketCount = 1;
            while (bracketCount > 0)
            {
                if (current().getTokenType() == lexing::TokenType::LeftBracket)
                    bracketCount++;
                else if (current().getTokenType() == lexing::TokenType::RightBracket)
                    bracketCount--;

                consume();
            }
        }

        if (exported)
        {
            mSymbols.push_back({name, type});
            return std::make_unique<Function>(std::move(attributes), type, std::move(arguments), std::move(name), std::vector<ASTNodePtr>(), nullptr);
        }
        return nullptr;
    }

    NamespacePtr ImportParser::parseNamespace()
    {
        consume(); // namespace

        expectToken(lexing::TokenType::Identifier);
        std::string name = consume().getText();
        mNamespaces.push_back(name);

        expectToken(lexing::TokenType::LeftBracket);
        consume();

        Scope* scope = new Scope(mScope, nullptr);
        mScope = scope;
        
        std::vector<ASTNodePtr> body;
        while(current().getTokenType() != lexing::TokenType::RightBracket)
        {
            ASTNodePtr node = parseGlobal(body);
            if (node)
            {
                body.push_back(std::move(node));
            }
        }
        consume();

        mScope = scope->parent;
        mNamespaces.pop_back();

        mSymbols.push_back({name, nullptr});
        return std::make_unique<Namespace>(std::move(name), std::move(body), scope);
    }

    StructDeclarationPtr ImportParser::parseStructDeclaration(bool exported)
    {
        consume(); // struct

        expectToken(lexing::TokenType::Identifier);
        std::string name = consume().getText();
        std::vector<std::string> names = mNamespaces;
        names.push_back(name);

        expectToken(lexing::TokenType::LeftBracket);
        consume();

        StructType* structType = StructType::Create(names, {});

        std::vector<StructType::Field>& fieldTypes = structType->getFields();
        std::vector<StructField> fields;
        std::vector<StructMethod> methods;
        while (current().getTokenType() != lexing::TokenType::RightBracket)
        {
            bool priv = false;
            if (current().getTokenType() == lexing::TokenType::PrivateKeyword)
            {
                consume();
                priv = true;
            }

            if (current().getTokenType() == lexing::TokenType::FuncKeyword)
            {
                consume();
                expectToken(lexing::TokenType::Asperand);
                consume();

                expectToken(lexing::TokenType::Identifier);
                std::string name = consume().getText();

                expectToken(lexing::TokenType::LeftParen);
                consume();

                std::vector<FunctionArgument> arguments;
                while (current().getTokenType() != lexing::TokenType::RightParen)
                {
                    expectToken(lexing::TokenType::Identifier);
                    std::string name = consume().getText();

                    expectToken(lexing::TokenType::Colon);
                    consume();

                    Type* type = parseType();
                    arguments.push_back({std::move(name), type});

                    if (current().getTokenType() != lexing::TokenType::RightParen)
                    {
                        expectToken(lexing::TokenType::Comma);
                        consume();
                    }
                }
                consume();

                Type* returnType = Type::Get("void");
                if (current().getTokenType() == lexing::TokenType::RightArrow)
                {
                    consume();
                    returnType = parseType();
                }

                std::vector<Type*> argumentTypes { PointerType::Create(structType) };
                for (auto& argument : arguments)
                {
                    argumentTypes.push_back(argument.type);
                }
                Type* type = FunctionType::Create(returnType, std::move(argumentTypes));

                if (current().getTokenType() == lexing::TokenType::Semicolon)
                {
                    consume();
                    methods.push_back({priv, std::move(name), type, std::move(arguments), std::vector<ASTNodePtr>(), nullptr});
                    continue;
                }

                expectEitherToken({lexing::TokenType::LeftBracket, lexing::TokenType::Equals});
                bool isExpressionBodied = current().getTokenType() == lexing::TokenType::Equals;
                consume();

                if (isExpressionBodied)
                {
                    while (current().getTokenType() != lexing::TokenType::Semicolon)
                        consume();
                    consume();
                }
                else
                {
                    int bracketCount = 1;
                    while (bracketCount > 0)
                    {
                        if (current().getTokenType() == lexing::TokenType::LeftBracket)
                            bracketCount++;
                        else if (current().getTokenType() == lexing::TokenType::RightBracket)
                            bracketCount--;

                        consume();
                    }
                }

                methods.push_back({priv, std::move(name), type, std::move(arguments), std::vector<ASTNodePtr>(), nullptr});
            }
            else
            {
                expectToken(lexing::TokenType::Identifier);
                std::string name = consume().getText();

                expectToken(lexing::TokenType::Colon);
                consume();

                Type* type = parseType();

                fieldTypes.push_back({priv, name, type});
                fields.push_back({priv, std::move(name), type});

                expectToken(lexing::TokenType::Semicolon);
                consume();
            }
        }
        consume();

        auto decl = std::make_unique<StructDeclaration>(std::move(names), std::move(fields), std::move(methods), structType);
        if (!exported)
            mStructTypesToRemove.push_back(decl->getType());
        return std::move(decl);
    }

    GlobalDeclarationPtr ImportParser::parseGlobalDeclaration(bool exported)
    {
        consume(); // global

        expectToken(lexing::TokenType::Identifier);
        std::vector<std::string> names = mNamespaces;
        names.push_back(consume().getText());

        expectToken(lexing::TokenType::Colon);
        consume();

        Type* type = parseType();

        expectToken(lexing::TokenType::Equals);
        consume();

        while(current().getTokenType() != lexing::TokenType::Semicolon)
        {
            consume();
        }
        consume();

        if (exported)
        {
            mSymbols.push_back({names.back(), type});
            return std::make_unique<GlobalDeclaration>(std::move(names), type, nullptr); // TODO: Extern
        }
        return nullptr;
    }

    ConstexprStatementPtr ImportParser::parseConstExpr(bool exported)
    {
        consume(); // constexpr

        expectToken(lexing::TokenType::Identifier);
        lexing::Token token = current();
        std::vector<std::string> names = mNamespaces;
        names.push_back(consume().getText());

        expectToken(lexing::TokenType::Colon);
        consume();

        Type* type = parseType();

        expectToken(lexing::TokenType::Equals);
        consume();

        while(current().getTokenType() != lexing::TokenType::Semicolon)
        {
            consume();
        }
        consume();

        if (exported)
        {
            mSymbols.push_back({names.back(), type});
            return std::make_unique<ConstexprStatement>(type, std::move(names), nullptr, token, true);
        }
        return nullptr;
    }

    std::pair<std::vector<ASTNodePtr>, std::vector<GlobalSymbol>> ImportParser::parseImportStatement(bool exported)
    {
        consume(); // import

        std::filesystem::path path;
        while (current().getTokenType() != lexing::TokenType::Semicolon)
        {
            expectToken(lexing::TokenType::Identifier);
            path /= consume().getText();

            if (current().getTokenType() != lexing::TokenType::Semicolon)
            {
                expectToken(lexing::TokenType::Dot);
                consume();
            }
        }
        consume();

        if (exported)
        {
            return mImportManager.ImportSymbols(path, mDiag);
        }
        return {};
    }
    
    UsingDeclarationPtr ImportParser::parseUsingDeclaration(bool exported)
    {
        consume(); // using

        std::vector<std::string> names = mNamespaces;
        names.push_back(consume().getText());

        expectToken(lexing::TokenType::Equals);
        consume();

        Type* type = parseType();

        expectToken(lexing::TokenType::Semicolon);
        consume();

        if (exported)
            return std::make_unique<UsingDeclaration>(std::move(names), type);

        return nullptr;
    }



    EnumDeclarationPtr ImportParser::parseEnumDeclaration(bool exported, std::vector<GlobalAttribute> attributes)
    {
        consume(); // enum

        std::vector<std::string> names = mNamespaces;
        names.push_back(consume().getText());

        expectToken(lexing::TokenType::LeftBracket);
        consume();

        std::vector<EnumField> fields;
        int currentValue = 0;
        while (current().getTokenType() != lexing::TokenType::RightBracket)
        {
            expectToken(lexing::TokenType::Identifier);
            std::string name = consume().getText();

            if (current().getTokenType() == lexing::TokenType::Equals)
            {
                consume();
                expectToken(lexing::TokenType::IntegerLiteral);
                currentValue = std::stoi(consume().getText(), 0, 0);
            }

            fields.push_back({std::move(name), currentValue++});

            if (current().getTokenType() != lexing::TokenType::RightBracket)
            {
                expectToken(lexing::TokenType::Comma);
                consume();
            }
        }
        consume();
        
        if (exported)
        {
            mSymbols.push_back({names.back(), nullptr});
            for (auto field : fields)
            {
                mSymbols.push_back({field.name, nullptr});
            }
            return std::make_unique<EnumDeclaration>(std::move(attributes), std::move(names), std::move(fields));
        }
        return nullptr;
    }

    void ImportParser::parseAttributes(std::vector<GlobalAttribute>& attributes)
    {
        consume(); // [[

        while (current().getTokenType() != lexing::TokenType::DoubleRightSquareBracket)
        {
            lexing::Token token = consume();

            if (token.getText() == "NoMangle")
            {
                attributes.push_back(GlobalAttribute(GlobalAttributeType::NoMangle));
            }
            else if (token.getText() == "GenerateNames")
            {
                attributes.push_back(GlobalAttribute(GlobalAttributeType::GenerateNames));
            }
            else
            {
                mDiag.compilerError(token.getStart(), token.getEnd(), std::format("unknown attribute '{}{}{}'", fmt::bold, token.getText(), fmt::defaults));
            }

            if (current().getTokenType() != lexing::TokenType::DoubleRightSquareBracket)
            {
                expectToken(lexing::TokenType::Comma);
                consume();
            }
        }
        consume();
    }
}
