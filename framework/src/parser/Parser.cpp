// Copyright 2023 solar-mist


#include "parser/Parser.h"

#include "parser/ast/expression/BinaryExpression.h"
#include "parser/ast/expression/UnaryExpression.h"
#include "parser/ast/expression/BooleanLiteral.h"
#include "parser/ast/expression/NullptrLiteral.h"
#include "parser/ast/expression/CastExpression.h"
#include "parser/ast/expression/ScopeResolution.h"
#include "parser/ast/statement/BreakStatement.h"
#include "parser/ast/statement/ContinueStatement.h"

#include "lexer/Token.h"

#include "symbol/Identifier.h"
#include "symbol/Import.h"

#include "type/PointerType.h"
#include "type/StructType.h"
#include "type/ArrayType.h"

#include <algorithm>
#include <filesystem>
#include <format>

#include "parser/ImportParser.h"

namespace parser
{
    Parser::Parser(std::vector<lexing::Token>& tokens, diagnostic::Diagnostics& diag, symbol::ImportManager& importManager)
        : mTokens(tokens)
        , mImportManager(importManager)
        , mPosition(0)
        , mScope(nullptr)
        , mDiag(diag)
    {
    }

    lexing::Token Parser::current() const
    {
        return mTokens.at(mPosition);
    }

    lexing::Token Parser::consume()
    {
        return mTokens.at(mPosition++);
    }

    lexing::Token Parser::peek(int offset) const
    {
        return mTokens.at(mPosition + offset);
    }

    void Parser::expectToken(lexing::TokenType tokenType)
    {
        if (current().getTokenType() != tokenType)
        {
            lexing::Token temp(tokenType, {0, 0}, {0, 0});
            mDiag.compilerError(current().getStart(), current().getEnd(), std::format("expected '{}{}{}' before '{}{}{}' token",
                fmt::bold, temp.getId(), fmt::defaults, fmt::bold, current().getId(), fmt::defaults));
        }
    }

    void Parser::expectEitherToken(std::vector<lexing::TokenType> tokenTypes)
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

    std::vector<ASTNodePtr> Parser::parse()
    {
        std::vector<ASTNodePtr> result;

        auto tokensCopy = mTokens;

        ImportParser hoistingParser(tokensCopy, mDiag, mImportManager, true);

        auto nodes = hoistingParser.parse();
        auto symbols = hoistingParser.getSymbols();

        std::move(nodes.begin(), nodes.end(), std::back_inserter(result));
        std::move(symbols.begin(), symbols.end(), std::back_inserter(mSymbols));

        while (mPosition < mTokens.size())
        {
            auto node = parseGlobal(result);
            if (node)
            {
                result.push_back(std::move(node));
            }
        }

        return result;
    }

    ASTNodePtr Parser::parseGlobal(std::vector<ASTNodePtr>& nodes)
    {
        std::vector<GlobalAttribute> attributes;
        if (current().getTokenType() == lexing::TokenType::DoubleLeftSquareBracket)
        {
            parseAttributes(attributes);
        }

        if (current().getTokenType() == lexing::TokenType::ExportKeyword)
        {
            consume();
            expectEitherToken({ lexing::TokenType::FuncKeyword, lexing::TokenType::GlobalKeyword,
                                lexing::TokenType::ConstexprKeyword, lexing::TokenType::StructKeyword,
                                lexing::TokenType::UsingKeyword, lexing::TokenType::EnumKeyword });
        }

        switch (current().getTokenType())
        {
            case lexing::TokenType::FuncKeyword:
                return parseFunction(attributes);
            case lexing::TokenType::StructKeyword:
                return parseStructDeclaration();
            case lexing::TokenType::GlobalKeyword:
                return parseGlobalDeclaration();
            case lexing::TokenType::ConstexprKeyword:
                return parseConstexprStatement(true);
            case lexing::TokenType::ImportKeyword:
            {
                auto symbols = parseImportStatement();
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
                    StructDeclarationPtr structDecl = parseStructDeclaration();
                    Type::AddAlias(structDecl->getNames(), structDecl->getType());
                    return structDecl;
                }
                return parseUsingDeclaration();
                
            case lexing::TokenType::EnumKeyword:
                return parseEnumDeclaration();
            default:
                mDiag.compilerError(current().getStart(), current().getEnd(), "Unexpected token. Expected global statement");
        }
    }

    int Parser::getBinaryOperatorPrecedence(lexing::TokenType tokenType)
    {
        switch (tokenType)
        {
            case lexing::TokenType::DoubleColon:
                return 100;

            case lexing::TokenType::LeftParen:
            case lexing::TokenType::LeftSquareBracket:
            case lexing::TokenType::Dot:
            case lexing::TokenType::RightArrow:
                return 90;

            case lexing::TokenType::Star:
            case lexing::TokenType::Slash:
                return 75;

            case lexing::TokenType::Plus:
            case lexing::TokenType::Minus:
                return 70;

            case lexing::TokenType::LessThan:
            case lexing::TokenType::GreaterThan:
            case lexing::TokenType::LessEqual:
            case lexing::TokenType::GreaterEqual:
                return 55;

            case lexing::TokenType::DoubleEquals:
            case lexing::TokenType::BangEquals:
                return 50;

            case lexing::TokenType::Ampersand:
                return 45;
            case lexing::TokenType::Caret:
                return 40;
            case lexing::TokenType::Pipe:
                return 35;

            case lexing::TokenType::Equals:
            case lexing::TokenType::PlusEquals:
            case lexing::TokenType::MinusEquals:
                return 20;

            default:
                return 0;
        }
    }

    int Parser::getPrefixUnaryOperatorPrecedence(lexing::TokenType tokenType)
    {
        switch(tokenType)
        {
            // case lexing::TokenType::DoublePlus:
            // case lexing::TokenType::DoubleMinus:
            case lexing::TokenType::Minus:
            case lexing::TokenType::Tilde:
            case lexing::TokenType::Ampersand:
            case lexing::TokenType::Star:
            case lexing::TokenType::LeftParen:
                return 85;

            default:
                return 0;
        }
    }

    int Parser::getPostfixUnaryOperatorPrecedence(lexing::TokenType tokenType)
    {
        switch(tokenType)
        {
            case lexing::TokenType::DoublePlus:
            case lexing::TokenType::DoubleMinus:
                return 95;
            default:
                return 0;
        }
    }

    Type* Parser::parseType(bool failable)
    {
        int startPosition = mPosition;
        Type* type = nullptr;
        if (current().getTokenType() == lexing::TokenType::StructKeyword)
        {
            consume();
            std::vector<std::string> names;
            expectToken(lexing::TokenType::Identifier);
            while (current().getTokenType() == lexing::TokenType::Identifier)
            {
                names.push_back(consume().getText());
                if (current().getTokenType() == lexing::TokenType::DoubleColon)
                {
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
                if (failable)
                {
                    mPosition = startPosition;
                    return nullptr;
                }
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
                    if (current().getTokenType() == lexing::TokenType::DoubleColon)
                    {
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
                if (!names.empty())
                    type = Type::Get(names.front());

                if (!type)
                {
                    if (failable)
                    {
                        mPosition = startPosition;
                        return nullptr;
                    }

                    mDiag.compilerError(token.getStart(), token.getEnd(), std::format("unknown type name '{}{}{}'", fmt::bold, token.getText(), fmt::defaults));
                }
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

    ASTNodePtr Parser::parseExpression(Type* preferredType, int precedence)
    {
        ASTNodePtr lhs;
        int prefixOperatorPrecedence = getPrefixUnaryOperatorPrecedence(current().getTokenType());
        if (prefixOperatorPrecedence >= precedence)
        {
            lexing::Token operatorToken = consume();
            if (operatorToken.getTokenType() == lexing::TokenType::LeftParen) // Cast expression or parenthesized expression
            {
                if (Type* type = parseType(true)) // Cast
                {
                    expectToken(lexing::TokenType::RightParen);
                    consume();
                    lhs = std::make_unique<CastExpression>(parseExpression(nullptr, prefixOperatorPrecedence), type, std::move(operatorToken));
                }
                else // Parenthesized expression
                {
                    lhs = parseParenthesizedExpression(preferredType);
                }
            }
            else
            {
                lhs = std::make_unique<UnaryExpression>(parseExpression(preferredType, prefixOperatorPrecedence), std::move(operatorToken));
            }
        }
        else
        {
            lhs = parsePrimary(preferredType);
        }

        while (true)
        {
            int postfixOperatorPrecedence = getPostfixUnaryOperatorPrecedence(current().getTokenType());
            if (postfixOperatorPrecedence < precedence)
            {
                break;
            }

            lexing::Token operatorToken = consume();

            lhs = std::make_unique<UnaryExpression>(std::move(lhs), std::move(operatorToken), true);
        }

        while (true)
        {
            int binaryOperatorPrecedence = getBinaryOperatorPrecedence(current().getTokenType());
            if (binaryOperatorPrecedence < precedence)
            {
                break;
            }

            lexing::Token operatorToken = consume();

            if (operatorToken.getTokenType() == lexing::TokenType::LeftParen)
            {
                lhs = parseCallExpression(std::move(lhs));
            }
            else if (operatorToken.getTokenType() == lexing::TokenType::Dot)
            {
                lhs = parseMemberAccess(std::move(lhs), false);
            }
            else if (operatorToken.getTokenType() == lexing::TokenType::RightArrow)
            {
                lhs = parseMemberAccess(std::move(lhs), true);
            }
            else if (operatorToken.getTokenType() == lexing::TokenType::DoubleColon)
            {
                lexing::Token token = current();
                lhs = std::make_unique<ScopeResolution>(std::move(lhs), token, parseExpression(nullptr, binaryOperatorPrecedence));
            }
            else
            {
                ASTNodePtr rhs = parseExpression(nullptr, binaryOperatorPrecedence);
                lhs = std::make_unique<BinaryExpression>(std::move(lhs), std::move(operatorToken), std::move(rhs));
            }

            if (operatorToken.getTokenType() == lexing::TokenType::LeftSquareBracket)
            {
                expectToken(lexing::TokenType::RightSquareBracket);
                consume();
            }
        }

        return lhs;
    }

    ASTNodePtr Parser::parsePrimary(Type* preferredType)
    {
        switch (current().getTokenType())
        {
            case lexing::TokenType::LeftBracket:
                return parseCompoundStatement();

            case lexing::TokenType::ReturnKeyword:
                return parseReturnStatement();

            case lexing::TokenType::LetKeyword:
                return parseVariableDeclaration();

            case lexing::TokenType::ConstexprKeyword:
                return parseConstexprStatement(false);

            case lexing::TokenType::IfKeyword:
                return parseIfStatement();
            case lexing::TokenType::WhileKeyword:
                return parseWhileStatement();
            case lexing::TokenType::ForKeyword:
                return parseForStatement();
            case lexing::TokenType::SwitchKeyword:
                return parseSwitchStatement();
            case lexing::TokenType::BreakKeyword:
                return std::make_unique<BreakStatement>(std::move(consume()));
            case lexing::TokenType::ContinueKeyword:
                return std::make_unique<ContinueStatement>(std::move(consume()));

            case lexing::TokenType::TrueKeyword:
                return std::make_unique<BooleanLiteral>(true, consume());
            case lexing::TokenType::FalseKeyword:
                return std::make_unique<BooleanLiteral>(false, consume());

            case lexing::TokenType::NullptrKeyword:
                return std::make_unique<NullptrLiteral>(preferredType, consume());

            case lexing::TokenType::SizeofKeyword:
                return parseSizeof(preferredType);

            case lexing::TokenType::IntegerLiteral:
                return parseIntegerLiteral(preferredType);
            case lexing::TokenType::StringLiteral:
                return parseStringLiteral();

            case lexing::TokenType::Identifier:
            {
                lexing::Token token = current();
                if (auto type = parseType(true))
                {
                    return parseStructInitializer(type, std::move(token));
                }
                return parseVariableExpression(preferredType);
            }

            case lexing::TokenType::StructKeyword:
                return parseStructInitializer(nullptr, lexing::Token());

            case lexing::TokenType::LeftSquareBracket:
                return parseArrayInitializer(preferredType);

            default:
                mDiag.compilerError(current().getStart(), current().getEnd(), "Unexpected token. Expected primary expression");
        }
    }

    ASTNodePtr Parser::parseParenthesizedExpression(Type* preferredType)
    {
        ASTNodePtr expression = parseExpression(preferredType);

        expectToken(lexing::TokenType::RightParen);
        consume();

        return expression;
    }

    FunctionPtr Parser::parseFunction(std::vector<GlobalAttribute> attributes)
    {
        consume();

        expectToken(lexing::TokenType::Asperand);
        consume();

        expectToken(lexing::TokenType::Identifier);
        std::string name = consume().getText();

        expectToken(lexing::TokenType::LeftParen);
        consume();

        std::vector<FunctionArgument> arguments;

        Scope* functionScope = new Scope(mScope, nullptr);
        mScope = functionScope;

        while (current().getTokenType() != lexing::TokenType::RightParen)
        {
            expectToken(lexing::TokenType::Identifier);
            std::string name = consume().getText();

            expectToken(lexing::TokenType::Colon);
            consume();

            Type* type = parseType();
            mScope->locals[name] = LocalSymbol(nullptr, type);
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

        mSymbols.push_back({name, type});

        if (current().getTokenType() == lexing::TokenType::Semicolon) // Extern function declaration
        {
            consume();
            mScope = functionScope->parent;
            delete functionScope;
            return std::make_unique<Function>(std::move(attributes), type, std::move(arguments), std::move(name), std::vector<ASTNodePtr>(), nullptr);
        }

        expectEitherToken({lexing::TokenType::LeftBracket, lexing::TokenType::Equals});
        bool isExpressionBodied = current().getTokenType() == lexing::TokenType::Equals;
        consume();

        std::vector<ASTNodePtr> body;
        if (isExpressionBodied)
        {
            ASTNodePtr exp = parseExpression(type);
            if (type->isVoidType())
                body.push_back(std::move(exp));
            else
                body.push_back(std::make_unique<ReturnStatement>(std::move(exp)));
            expectToken(lexing::TokenType::Semicolon);
            consume();
        }
        else
        {
            while (current().getTokenType() != lexing::TokenType::RightBracket)
            {
                body.push_back(parseExpression());
                expectToken(lexing::TokenType::Semicolon);
                consume();
            }
            consume();
        }

        mScope = functionScope->parent;

        return std::make_unique<Function>(std::move(attributes), type, std::move(arguments), std::move(name), std::move(body), functionScope);
    }

    NamespacePtr Parser::parseNamespace()
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
        mSymbols.push_back({name, nullptr});

        mScope = scope->parent;
        mNamespaces.pop_back();

        return std::make_unique<Namespace>(std::move(name), std::move(body), scope);
    }

    StructDeclarationPtr Parser::parseStructDeclaration()
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

                Scope* scope = new Scope(mScope, structType);
                mScope = scope;

                std::vector<FunctionArgument> arguments;
                while (current().getTokenType() != lexing::TokenType::RightParen)
                {
                    expectToken(lexing::TokenType::Identifier);
                    std::string name = consume().getText();

                    expectToken(lexing::TokenType::Colon);
                    consume();

                    Type* type = parseType();
                    mScope->locals[name] = LocalSymbol(nullptr, type);
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

                mScope->locals["this"] = LocalSymbol(nullptr, PointerType::Create(structType));

                expectEitherToken({lexing::TokenType::LeftBracket, lexing::TokenType::Equals});
                bool isExpressionBodied = current().getTokenType() == lexing::TokenType::Equals;
                consume();

                std::vector<ASTNodePtr> body;
                if (isExpressionBodied)
                {
                    ASTNodePtr exp = parseExpression(type);
                    if (type->isVoidType())
                        body.push_back(std::move(exp));
                    else
                        body.push_back(std::make_unique<ReturnStatement>(std::move(exp)));
                    expectToken(lexing::TokenType::Semicolon);
                    consume();
                }
                else
                {
                    while (current().getTokenType() != lexing::TokenType::RightBracket)
                    {
                        body.push_back(parseExpression());
                        expectToken(lexing::TokenType::Semicolon);
                        consume();
                    }
                    consume();
                }

                mScope = mScope->parent;

                methods.push_back({priv, name, type, std::move(arguments), std::move(body), ScopePtr(scope)});
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

        return std::make_unique<StructDeclaration>(std::move(names), std::move(fields), std::move(methods), structType);
    }

    GlobalDeclarationPtr Parser::parseGlobalDeclaration()
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

        ASTNodePtr initVal = parseExpression(type);

        expectToken(lexing::TokenType::Semicolon);
        consume();

        mSymbols.push_back({names.back(), type});

        return std::make_unique<GlobalDeclaration>(std::move(names), type, std::move(initVal));
    }

    std::pair<std::vector<ASTNodePtr>, std::vector<GlobalSymbol>> Parser::parseImportStatement()
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

        return mImportManager.ImportSymbols(path, mDiag);
    }
    
    UsingDeclarationPtr Parser::parseUsingDeclaration()
    {
        consume(); // using

        std::vector<std::string> names = mNamespaces;
        names.push_back(consume().getText());

        expectToken(lexing::TokenType::Equals);
        consume();

        Type* type = parseType();

        expectToken(lexing::TokenType::Semicolon);
        consume();

        return std::make_unique<UsingDeclaration>(std::move(names), type);
    }

    EnumDeclarationPtr Parser::parseEnumDeclaration()
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
                IntegerLiteralPtr literal = parseIntegerLiteral();
                currentValue = literal->getValue();
            }

            fields.push_back({std::move(name), currentValue++});

            if (current().getTokenType() != lexing::TokenType::RightBracket)
            {
                expectToken(lexing::TokenType::Comma);
                consume();
            }
        }
        consume();

        return std::make_unique<EnumDeclaration>(std::move(names), std::move(fields));
    }

    CompoundStatementPtr Parser::parseCompoundStatement()
    {
        consume(); // left bracket

        Scope* blockScope = new Scope(mScope, nullptr);
        mScope = blockScope;

        std::vector<ASTNodePtr> body;
        while (current().getTokenType() != lexing::TokenType::RightBracket)
        {
            body.push_back(parseExpression());
            expectToken(lexing::TokenType::Semicolon);
            consume();
        }
        consume();

        mTokens.insert(mTokens.begin()+mPosition, lexing::Token(lexing::TokenType::Semicolon, {0, 0}, {0, 0}));

        mScope = blockScope->parent;

        return std::make_unique<CompoundStatement>(std::move(body), blockScope);
    }

    ReturnStatementPtr Parser::parseReturnStatement()
    {
        consume();

        if (current().getTokenType() == lexing::TokenType::Semicolon)
        {
            return std::make_unique<ReturnStatement>(nullptr);
        }

        return std::make_unique<ReturnStatement>(parseExpression()); // TODO: Pass preferred type as current function return type
    }

    VariableDeclarationPtr Parser::parseVariableDeclaration()
    {
        consume(); // let

        std::string name = consume().getText();

        expectToken(lexing::TokenType::Colon);
        consume();

        Type* type = parseType();
        
        mScope->locals[name] = LocalSymbol(nullptr, type);

        if (current().getTokenType() == lexing::TokenType::Semicolon)
        {
            return std::make_unique<VariableDeclaration>(type, std::move(name), nullptr);
        }

        expectToken(lexing::TokenType::Equals);
        consume();

        return std::make_unique<VariableDeclaration>(type, std::move(name), parseExpression(type));
    }

    ConstexprStatementPtr Parser::parseConstexprStatement(bool global)
    {
        consume(); // constexpr

        lexing::Token token = current();
        std::string name = consume().getText();

        auto names = mNamespaces;
        names.push_back(name);

        expectToken(lexing::TokenType::Colon);
        consume();

        Type* type = parseType();

        if (global)
            mSymbols.push_back({name, type});
        else
            mScope->locals[name] = LocalSymbol(nullptr, type);

        expectToken(lexing::TokenType::Equals);
        consume();

        ASTNodePtr value = parseExpression(type);

        if (global)
        {
            expectToken(lexing::TokenType::Semicolon);
            consume();
        }

        return std::make_unique<ConstexprStatement>(type, global ? std::move(names) : std::vector<std::string>{name}, std::move(value), token, global);
    }

    IfStatementPtr Parser::parseIfStatement()
    {
        consume(); // if

        expectToken(lexing::TokenType::LeftParen);
        consume();

        ASTNodePtr condition = parseExpression();

        expectToken(lexing::TokenType::RightParen);
        consume();

        ASTNodePtr body = parseExpression();
        ASTNodePtr elseBody = nullptr;

        if (peek(1).getTokenType() == lexing::TokenType::ElseKeyword)
        {
            expectToken(lexing::TokenType::Semicolon);
            consume();

            consume(); // else

            elseBody = parseExpression();
        }

        return std::make_unique<IfStatement>(std::move(condition), std::move(body), std::move(elseBody));
    }

    WhileStatementPtr Parser::parseWhileStatement()
    {
        consume(); // while

        expectToken(lexing::TokenType::LeftParen);
        consume();

        Scope* whileScope = new Scope(mScope, nullptr);
        mScope = whileScope;

        ASTNodePtr condition = parseExpression();

        expectToken(lexing::TokenType::RightParen);
        consume();

        ASTNodePtr body = parseExpression();

        mScope = whileScope->parent;

        return std::make_unique<WhileStatement>(std::move(condition), std::move(body), whileScope);
    }

    ForStatementPtr Parser::parseForStatement()
    {
        consume();

        expectToken(lexing::TokenType::LeftParen);
        consume();

        ASTNodePtr init = nullptr;
        ASTNodePtr condition = nullptr;
        std::vector<ASTNodePtr> loopExpr;

        Scope* forScope = new Scope(mScope, nullptr);
        mScope = forScope;

        if (current().getTokenType() != lexing::TokenType::Semicolon)
        {
            init = parseExpression();
            expectToken(lexing::TokenType::Semicolon);
        }
        consume();

        if (current().getTokenType() != lexing::TokenType::Semicolon)
        {
            condition = parseExpression();
            expectToken(lexing::TokenType::Semicolon);
        }
        consume();

        if (current().getTokenType() != lexing::TokenType::RightParen)
        {
            while (current().getTokenType() != lexing::TokenType::RightParen)
            {
                loopExpr.push_back(parseExpression());

                if (current().getTokenType() != lexing::TokenType::RightParen)
                {
                    expectToken(lexing::TokenType::Comma);
                    consume();
                }
            }
        }
        consume();

        ASTNodePtr body = parseExpression();

        mScope = forScope->parent;

        return std::make_unique<ForStatement>(std::move(init), std::move(condition), std::move(loopExpr), std::move(body), forScope);
    }

    SwitchStatementPtr Parser::parseSwitchStatement()
    {
        consume();

        expectToken(lexing::TokenType::LeftParen);
        consume();

        static auto isSwitchableType = [](Type* type)
        {
            return type &&
                   (type->isEnumType() ||
                   type->isIntegerType());
        };

        lexing::Token valueToken = current();
        ASTNodePtr value = parseExpression();
        if (!isSwitchableType(value->getType()))
        {
            mDiag.compilerError(valueToken.getStart(), valueToken.getEnd(), "Expression must have integral or enum type");
        }

        expectToken(lexing::TokenType::RightParen);
        consume();

        expectToken(lexing::TokenType::LeftBracket);
        consume();

        std::vector<SwitchSection> sections;
        bool hasDefaultSection = false;

        while (current().getTokenType() != lexing::TokenType::RightBracket)
        {
            lexing::Token sectionToken = current();
            expectEitherToken({lexing::TokenType::CaseKeyword, lexing::TokenType::DefaultKeyword});
            bool defSection = current().getTokenType() == lexing::TokenType::DefaultKeyword;
            consume();

            if (hasDefaultSection && defSection)
            {
                mDiag.compilerError(sectionToken.getStart(), sectionToken.getEnd(), "Default label has already appeared in this switch");
            }
            hasDefaultSection |= defSection;

            lexing::Token labelToken = current();
            ASTNodePtr label = defSection ? nullptr : parseExpression();
            if (label && !isSwitchableType(label->getType()))
            {
                mDiag.compilerError(labelToken.getStart(), labelToken.getEnd(), "Expression must have integral or enum type");
            }

            expectToken(lexing::TokenType::Colon);
            consume();

            std::vector<ASTNodePtr> body;
            while (current().getTokenType() != lexing::TokenType::RightBracket &&
                current().getTokenType() != lexing::TokenType::CaseKeyword &&
                current().getTokenType() != lexing::TokenType::DefaultKeyword)
            {
                body.push_back(parseExpression());
                if (current().getTokenType() != lexing::TokenType::RightBracket) // Switch end }
                {
                    expectToken(lexing::TokenType::Semicolon);
                    consume();
                }
            }

            sections.push_back({std::move(label), std::move(body)});
        }
        consume();

        mTokens.insert(mTokens.begin() + mPosition, lexing::Token(lexing::TokenType::Semicolon, {0, 0}, {0, 0}));

        return std::make_unique<SwitchStatement>(std::move(value), std::move(sections));
    }

    SizeofExpressionPtr Parser::parseSizeof(Type* preferredType)
    {
        lexing::Token token = consume();
        expectToken(lexing::TokenType::LeftParen);
        consume();

        Type* type = parseType();

        expectToken(lexing::TokenType::RightParen);
        consume();

        return std::make_unique<SizeofExpression>(preferredType, type, std::move(token));
    }

    IntegerLiteralPtr Parser::parseIntegerLiteral(Type* preferredType)
    {
        lexing::Token token = consume();
        unsigned long long value = std::stoull(token.getText(), 0, 0);
        return std::make_unique<IntegerLiteral>(value, preferredType, std::move(token));
    }

    StringLiteralPtr Parser::parseStringLiteral()
    {
        lexing::Token token = consume();
        std::string text = token.getText();
        return std::make_unique<StringLiteral>(std::move(text), std::move(token));
    }

    VariableExpressionPtr Parser::parseVariableExpression(Type*)
    {
        lexing::Token nameToken = current();
        std::string name = consume().getText();

        auto local = mScope->findVariable(name);
        if (local)
        {
            return std::make_unique<VariableExpression>(std::move(name), local->type, std::move(nameToken));
        }

        if (mScope->findOwner() != nullptr && mScope->findOwner()->hasField(name))
        {
            std::string nameCopy = name;
            return std::make_unique<VariableExpression>(std::move(nameCopy), mScope->findOwner()->getField(name)->type, std::move(nameToken));
        }

        auto it = std::find_if(mSymbols.begin(), mSymbols.end(), [&name](const GlobalSymbol& symbol) {
            return symbol.name == name;
        });

        if (it != mSymbols.end())
        {
            return std::make_unique<VariableExpression>(std::move(name), it->type, std::move(nameToken));
        }

        mDiag.compilerError(nameToken.getStart(), nameToken.getEnd(), std::format("Unknown symbol '{}{}{}'", fmt::bold, name, fmt::defaults));
    }

    CallExpressionPtr Parser::parseCallExpression(ASTNodePtr function)
    {
        lexing::Token token = peek(-1); // left paren

        std::vector<ASTNodePtr> parameters;
        while (current().getTokenType() != lexing::TokenType::RightParen)
        {
            parameters.push_back(parseExpression());

            if (current().getTokenType() != lexing::TokenType::RightParen)
            {
                expectToken(lexing::TokenType::Comma);
                consume();
            }
        }
        consume();

        return std::make_unique<CallExpression>(std::move(function), std::move(parameters), std::move(token));
    }

    MemberAccessPtr Parser::parseMemberAccess(ASTNodePtr struc, bool pointer)
    {
        lexing::Token nameToken = consume();

        return std::make_unique<MemberAccess>(std::move(struc), std::string(nameToken.getText()), pointer, std::move(nameToken));
    }

    StructInitializerPtr Parser::parseStructInitializer(Type* type, lexing::Token token)
    {
        if (current().getTokenType() == lexing::TokenType::StructKeyword)
        {
            token = current();
            type = parseType();
        }

        StructType* structType = static_cast<StructType*>(type);

        expectToken(lexing::TokenType::LeftBracket);
        consume();

        std::vector<ASTNodePtr> body;
        int index = 0;
        while (current().getTokenType() != lexing::TokenType::RightBracket)
        {
            body.push_back(parseExpression(structType->getFields()[index++].type));

            if (current().getTokenType() != lexing::TokenType::RightBracket)
            {
                expectToken(lexing::TokenType::Comma);
                consume();
            }
        }
        consume();

        return std::make_unique<StructInitializer>(type, std::move(body), std::move(token));
    }

    ArrayInitializerPtr Parser::parseArrayInitializer(Type* preferredType)
    {
        lexing::Token token = consume(); // left square bracket

        preferredType = static_cast<ArrayType*>(preferredType)->getBaseType();

        std::vector<ASTNodePtr> values;
        while (current().getTokenType() != lexing::TokenType::RightSquareBracket)
        {
            values.push_back(parseExpression(preferredType));

            if (current().getTokenType() != lexing::TokenType::RightSquareBracket)
            {
                expectToken(lexing::TokenType::Comma);
                consume();
            }
        }
        consume();

        return std::make_unique<ArrayInitializer>(std::move(values), std::move(token));
    }

    void Parser::parseAttributes(std::vector<GlobalAttribute>& attributes)
    {
        consume(); // [[

        while (current().getTokenType() != lexing::TokenType::DoubleRightSquareBracket)
        {
            lexing::Token token = consume();

            if (token.getText() == "NoMangle")
            {
                attributes.push_back(GlobalAttribute(GlobalAttributeType::NoMangle));
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
