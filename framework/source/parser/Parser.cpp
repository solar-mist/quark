// Copyright 2024 solar-mist

#include "parser/Parser.h"

#include "parser/ast/expression/BooleanLiteral.h"

#include "type/PointerType.h"
#include "type/StructType.h"

#include <cinttypes>
#include <filesystem>
#include <format>
#include <ranges>

namespace parser
{
    Parser::Parser(std::vector<lexer::Token>& tokens, diagnostic::Diagnostics& diag, ImportManager& importManager, Scope* globalScope)
        : mTokens(tokens)
        , mPosition(0)
        , mDiag(diag)
        , mActiveScope(globalScope)
        , mExportBlock(false)
        , mImportManager(importManager)
    {
    }

    std::vector<ASTNodePtr> Parser::parse()
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

    lexer::Token Parser::current() const
    {
        return mTokens[mPosition];
    }

    lexer::Token Parser::consume()
    {
        return mTokens[mPosition++];
    }

    lexer::Token Parser::peek(int offset) const
    {
        return mTokens[mPosition + offset];
    }

    void Parser::expectToken(lexer::TokenType tokenType)
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

    int Parser::getBinaryOperatorPrecedence(lexer::TokenType tokenType) 
    {
        switch (tokenType) 
        {
            case lexer::TokenType::LeftParen:
            case lexer::TokenType::Dot:
            case lexer::TokenType::RightArrow:
                return 90;

            case lexer::TokenType::Star:
            case lexer::TokenType::Slash:
                return 75;
            case lexer::TokenType::Plus:
            case lexer::TokenType::Minus:
                return 70;

            case lexer::TokenType::LessThan:
            case lexer::TokenType::GreaterThan:
            case lexer::TokenType::LessEqual:
            case lexer::TokenType::GreaterEqual:
                return 55;

            case lexer::TokenType::DoubleEqual:
            case lexer::TokenType::BangEqual:
                return 50;

            case lexer::TokenType::Equal:
                return 20;

            default:
                return 0;
        }
    }

    int Parser::getPrefixUnaryOperatorPrecedence(lexer::TokenType tokenType) 
    {
        switch (tokenType) 
        {
            case lexer::TokenType::Minus:
            case lexer::TokenType::Ampersand:
            case lexer::TokenType::Star:
                return 85;
            default:
                return 0;
        }
    }

    int Parser::getPostfixUnaryOperatorPrecedence(lexer::TokenType tokenType) 
    {
        return 0;
    }

    Type* Parser::parseType()
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
        int save = mPosition;
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
        }
        if (!type) // No struct type was found
        {
            mPosition = save;
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


    ASTNodePtr Parser::parseGlobal(bool exported)
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
            {
                parseImport();
                return nullptr;
            }

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

            case lexer::TokenType::EnumKeyword:
                return parseEnum(exported);

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

    ASTNodePtr Parser::parseExpression(int precedence)
    {
        ASTNodePtr left;
        int prefixOperatorPrecedence = getPrefixUnaryOperatorPrecedence(current().getTokenType());

        if (prefixOperatorPrecedence >= precedence)
        {
            lexer::Token operatorToken = consume();
            left = std::make_unique<UnaryExpression>(mActiveScope, parseExpression(prefixOperatorPrecedence), operatorToken.getTokenType(), false, std::move(operatorToken));
        }
        else
        {
            left = parsePrimary();
        }

        while (true)
        {
            int postfixOperatorPrecedence = getPostfixUnaryOperatorPrecedence(current().getTokenType());
            if (postfixOperatorPrecedence < precedence) 
            {
                break;
            }

            lexer::Token operatorToken = consume();

            left = std::make_unique<UnaryExpression>(mActiveScope, std::move(left), operatorToken.getTokenType(), true, std::move(operatorToken));
        }

        while (true) 
        {
            int binaryOperatorPrecedence = getBinaryOperatorPrecedence(current().getTokenType());
            if (binaryOperatorPrecedence < precedence) 
            {
                break;
            }

            lexer::Token operatorToken = consume();

            if (operatorToken.getTokenType() == lexer::TokenType::LeftParen)
            {
                left = parseCallExpression(std::move(left));
            }
            else if (operatorToken.getTokenType() == lexer::TokenType::Dot)
            {
                left = parseMemberAccess(std::move(left), false);
            }
            else if (operatorToken.getTokenType() == lexer::TokenType::RightArrow)
            {
                left = parseMemberAccess(std::move(left), true);
            }
            else
            {
                ASTNodePtr right = parseExpression(binaryOperatorPrecedence);
                left = std::make_unique<BinaryExpression>(mActiveScope, std::move(left), operatorToken.getTokenType(), std::move(right), std::move(operatorToken));
            }
        }

        return left;
    }

    ASTNodePtr Parser::parsePrimary()
    {
        switch (current().getTokenType())
        {
            case lexer::TokenType::ReturnKeyword:
                return parseReturnStatement();

            case lexer::TokenType::LetKeyword:
                return parseVariableDeclaration();

            case lexer::TokenType::IfKeyword:
                return parseIfStatement();

            case lexer::TokenType::IntegerLiteral:
                return parseIntegerLiteral();

            case lexer::TokenType::Identifier:
                return parseVariableExpression();

            case lexer::TokenType::StringLiteral:
                return parseStringLiteral();

            case lexer::TokenType::TrueKeyword:
                return std::make_unique<BooleanLiteral>(mActiveScope, true, consume());
            case lexer::TokenType::FalseKeyword:
                return std::make_unique<BooleanLiteral>(mActiveScope, false, consume());

            case lexer::TokenType::CastKeyword:
                return parseCastExpression();

            default:
                mDiag.reportCompilerError(
                    current().getStartLocation(),
                    current().getEndLocation(),
                    std::format("Expected primary expression. Found '{}{}{}'", fmt::bold, current().getText(), fmt::defaults)
                );
                std::exit(1);
        }
    }


    FunctionPtr Parser::parseFunction(bool pure, bool exported)
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
            body.push_back(parseExpression());
            expectToken(lexer::TokenType::Semicolon);
            consume();
        }
        consume();

        mActiveScope = scope->parent;

        return std::make_unique<Function>(exported, pure, std::move(name), functionType, std::move(arguments), std::move(body), std::move(scope), std::move(token));
    }

    NamespacePtr Parser::parseNamespace(bool exported)
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

    EnumDeclarationPtr Parser::parseEnum(bool exported)
    {
        consume(); // enum

        lexer::Token token = consume();
        std::string name = std::string(token.getText());

        Type* base = Type::Get("i32");

        if (current().getTokenType() == lexer::TokenType::Colon)
        {
            consume();
            base = parseType();
        }

        ScopePtr scope = std::make_unique<Scope>(mActiveScope, name, true);
        mActiveScope = scope.get();

        std::vector<EnumField> fields;
        int nextValue = 0;
        
        expectToken(lexer::TokenType::LeftBrace);
        consume();
        while (current().getTokenType() != lexer::TokenType::RightBrace)
        {
            expectToken(lexer::TokenType::Identifier);
            std::string name = std::string(consume().getText());
            int value = nextValue;

            if (current().getTokenType() == lexer::TokenType::Equal)
            {
                consume();
                expectToken(lexer::TokenType::IntegerLiteral);
                value = std::stoi(std::string(consume().getText()));
            }

            fields.push_back(EnumField{std::move(name), static_cast<unsigned long long>(value)});
            nextValue = value + 1;

            if (current().getTokenType() != lexer::TokenType::RightBrace)
            {
                expectToken(lexer::TokenType::Comma);
                consume();
            }
        }
        consume();

        mActiveScope = scope->parent;

        return std::make_unique<EnumDeclaration>(exported, false, std::move(name), std::move(fields), base, std::move(scope), std::move(token));
    }

    ClassDeclarationPtr Parser::parseClassDeclaration(bool exported)
    {
        auto token = consume(); // class

        expectToken(lexer::TokenType::Identifier);
        std::string name = std::string(consume().getText());

        expectToken(lexer::TokenType::LeftBrace);
        consume();

        ScopePtr scope = std::make_unique<Scope>(mActiveScope, name, true);
        mActiveScope = scope.get();

        std::vector<ClassField> fields;
        std::vector<ClassMethod> methods;
        while (current().getTokenType() != lexer::TokenType::RightBrace)
        {
            bool currentPrivate = true;
            if (current().getTokenType() == lexer::TokenType::PublicKeyword)
            {
                consume();
                currentPrivate = false;
            }
            if (current().getTokenType() == lexer::TokenType::PureKeyword)
            {
                consume();
                expectToken(lexer::TokenType::FuncKeyword);
                methods.push_back(parseClassMethod(currentPrivate, true));
            }
            else if (current().getTokenType() == lexer::TokenType::FuncKeyword)
            {
                methods.push_back(parseClassMethod(currentPrivate, false));
            }
            else // Field
            {
                expectToken(lexer::TokenType::Identifier);
                std::string fieldName = std::string(consume().getText());

                expectToken(lexer::TokenType::Colon);
                consume();

                Type* fieldType = parseType();
                fields.push_back(ClassField(currentPrivate, fieldType, std::move(fieldName)));

                if (current().getTokenType() != lexer::TokenType::RightBrace)
                {
                    expectToken(lexer::TokenType::Semicolon);
                    consume();
                }
            }
        }
        consume();

        mActiveScope = scope->parent;

        auto namespaces = mActiveScope->getNamespaces();
        namespaces.push_back(name);
        auto mangled = StructType::MangleName(namespaces);
        PendingStructType* pendingType = nullptr;
        if (auto type = Type::Get(mangled))
        {
            pendingType = static_cast<PendingStructType*>(type);
            auto& pendings = PendingStructType::GetPending();
            std::erase(pendings, pendingType);
        }
        
        auto classDef = std::make_unique<ClassDeclaration>(exported, false, std::move(name), std::move(fields), std::move(methods), std::move(scope), std::move(token));
        if (pendingType) pendingType->initComplete();
        return classDef;
    }

    ClassMethod Parser::parseClassMethod(bool priv, bool pure)
    {
        consume(); // func

        expectToken(lexer::TokenType::Identifier);
        lexer::Token token = consume();
        std::string name = std::string(token.getText());

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

        std::vector<ASTNodePtr> body;
        expectToken(lexer::TokenType::LeftBrace);
        consume();

        while (current().getTokenType() != lexer::TokenType::RightBrace)
        {
            body.push_back(parseExpression());
            expectToken(lexer::TokenType::Semicolon);
            consume();
        }
        consume();

        mActiveScope = scope->parent;

        return ClassMethod{
            priv, pure, std::move(name),
            functionType, std::move(arguments), std::move(body),
            std::move(scope), std::move(token)
        };
    }

    void Parser::parseImport()
    {
        consume(); // import

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

        std::vector<Import> allImports;
        mImportManager.collectAllImports(path, mTokens[0].getStartLocation().file, allImports);
        for (auto it = allImports.begin(); it != allImports.end(); ++it)
        {
            auto& import = *it;

            // Don't import the current file
            if (import.from == mTokens[0].getStartLocation().file) continue;
            // We only want to import each file once
            auto lookIt = std::find_if(allImports.begin(), allImports.end(), [import](const auto& imp){
                return imp.from == import.from;
            });
            if (lookIt != it) continue;

            ScopePtr scope = std::make_unique<Scope>(nullptr, "", true);

            auto nodes = mImportManager.resolveImports(import.from, import.to, scope.get(), true);
            for (auto& node : nodes)
            {
                mInsertNodeFn(node);
            }

            auto exports = mImportManager.getExports();
            std::vector<Export> invalid;
            for (auto& exp : exports)
            {
                if (!exp.symbol || !mImportManager.wasExportedTo(std::string(mTokens[0].getStartLocation().file), allImports, exp))
                {
                    invalid.push_back(exp);
                }
            }
            std::function<bool(Scope* scope, Symbol* sym)> erase;
            erase = [&erase](Scope* scope, Symbol* sym){
                auto it = std::find_if(scope->symbols.begin(), scope->symbols.end(), [sym](Symbol& symbol) {
                    return &symbol == sym;
                });
                if (it != scope->symbols.end())
                {
                    scope->symbols.erase(it);
                    return true;
                }
                for (auto child : scope->children)
                {
                    if (erase(child, sym)) return true;
                }
                return false;
            };
            for (auto& symbol : invalid)
            {
                erase(scope.get(), symbol.symbol);
            }

            mActiveScope->children.push_back(scope.get());
            for (auto& pendingStructType : mImportManager.getPendingStructTypeNames())
            {
                auto type = static_cast<PendingStructType*>(Type::Get(StructType::MangleName(pendingStructType)));
                auto sym = scope->resolveSymbol(pendingStructType);
                if (sym) type->initComplete();
                else type->initIncomplete();
            }

            mImportManager.seizeScope(std::move(scope));
            mImportManager.clearExports();
        }
    }


    ReturnStatementPtr Parser::parseReturnStatement()
    {
        auto token = consume(); // ReturnKeyword

        if (current().getTokenType() == lexer::TokenType::Semicolon)
        {
            return std::make_unique<ReturnStatement>(mActiveScope, nullptr, std::move(token));
        }

        return std::make_unique<ReturnStatement>(mActiveScope, parseExpression(), std::move(token));
    }

    VariableDeclarationPtr Parser::parseVariableDeclaration()
    {
        consume(); // LetKeyword

        expectToken(lexer::TokenType::Identifier);
        auto token = consume();
        std::string name = std::string(token.getText());

        // Implicitly typed
        if (current().getTokenType() == lexer::TokenType::Equal)
        {
            consume();
            ASTNodePtr initValue = parseExpression();
            return std::make_unique<VariableDeclaration>(mActiveScope, std::move(name), nullptr, std::move(initValue), std::move(token));
        }
        
        expectToken(lexer::TokenType::Colon);
        consume();

        auto type = parseType();

        ASTNodePtr initValue = nullptr;
        if (current().getTokenType() == lexer::TokenType::Equal)
        {
            consume();

            initValue = parseExpression();
        }

        return std::make_unique<VariableDeclaration>(mActiveScope, std::move(name), type, std::move(initValue), std::move(token));
    }

    IfStatementPtr Parser::parseIfStatement()
    {
        auto token = consume();

        expectToken(lexer::TokenType::LeftParen);
        consume();

        auto condition = parseExpression();

        expectToken(lexer::TokenType::RightParen);
        consume();

        auto body = parseExpression();
        ASTNodePtr elseBody = nullptr;

        ScopePtr scope = std::make_unique<Scope>(mActiveScope, "", false);
        mActiveScope = scope.get();

        if (peek(1).getTokenType() == lexer::TokenType::ElseKeyword)
        {
            expectToken(lexer::TokenType::Semicolon);
            consume();

            consume(); // ElseKeyword

            elseBody = parseExpression();
        }

        mActiveScope = scope->parent;

        return std::make_unique<IfStatement>(std::move(condition), std::move(body), std::move(elseBody), std::move(scope), std::move(token));
    }


    IntegerLiteralPtr Parser::parseIntegerLiteral()
    {
        auto token = consume();
        std::string text = std::string(token.getText());

        return std::make_unique<IntegerLiteral>(mActiveScope, std::strtoimax(text.c_str(), nullptr, 0), std::move(token));
    }

    VariableExpressionPtr Parser::parseVariableExpression()
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

    CallExpressionPtr Parser::parseCallExpression(ASTNodePtr callee)
    {
        std::vector<ASTNodePtr> parameters;
        while (current().getTokenType() != lexer::TokenType::RightParen)
        {
            parameters.push_back(parseExpression());
            if (current().getTokenType() != lexer::TokenType::RightParen)
            {
                expectToken(lexer::TokenType::Comma);
                consume();
            }
        }
        consume();

        return std::make_unique<CallExpression>(mActiveScope, std::move(callee), std::move(parameters));
    }

    StringLiteralPtr Parser::parseStringLiteral()
    {
        auto token = consume();
        std::string text = std::string(token.getText());
        // Remove quotes
        text = text.substr(1);
        text.pop_back();

        return std::make_unique<StringLiteral>(mActiveScope, std::move(text), std::move(token));
    }

    MemberAccessPtr Parser::parseMemberAccess(ASTNodePtr structNode, bool pointer)
    {
        expectToken(lexer::TokenType::Identifier);
        auto token = consume();
        std::string text = std::string(token.getText());

        return std::make_unique<MemberAccess>(std::move(structNode), std::move(text), pointer, mActiveScope, peek(-2), std::move(token));
    }

    CastExpressionPtr Parser::parseCastExpression()
    {
        consume(); // cast

        expectToken(lexer::TokenType::LessThan);
        consume();

        auto destType = parseType();

        expectToken(lexer::TokenType::GreaterThan);
        consume();
        expectToken(lexer::TokenType::LeftParen);
        consume();
        auto expr = parseExpression();
        expectToken(lexer::TokenType::RightParen);
        consume();

        return std::make_unique<CastExpression>(mActiveScope, std::move(expr), destType);
    }
}