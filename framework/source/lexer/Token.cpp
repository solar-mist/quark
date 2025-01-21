// Copyright 2024 solar-mist

#include "lexer/Token.h"

namespace lexer
{
    Token::Token(std::string text, TokenType tokenType, SourceLocation start, SourceLocation end)
        : mText(std::move(text))
        , mTokenType(tokenType)
        , mStartLoc(start)
        , mEndLoc(end)
    {
    }

    std::string_view Token::getText() const
    {
        return mText;
    }

    std::string_view Token::getName() const
    {
        switch (mTokenType)
        {
            case TokenType::FuncKeyword:
                return "func";
            case TokenType::ReturnKeyword:
                return "return";
            case TokenType::TypeKeyword:
                return "a type";
            case TokenType::LetKeyword:
                return "let";
            case TokenType::IfKeyword:
                return "if";
            case TokenType::ElseKeyword:
                return "else";
            case TokenType::TrueKeyword:
                return "true";
            case TokenType::FalseKeyword:
                return "false";
            case TokenType::PureKeyword:
                return "pure";
            case TokenType::ExportKeyword:
                return "export";
            case TokenType::ImportKeyword:
                return "import";
            case TokenType::ClassKeyword:
                return "class";
            case TokenType::NamespaceKeyword:
                return "namespace";
            case TokenType::Plus:
                return "+";
            case TokenType::Minus:
                return "-";
            case TokenType::Star:
                return "*";
            case TokenType::Slash:
                return "/";
            case TokenType::DoubleEqual:
                return "==";
            case TokenType::BangEqual:
                return "!=";
            case TokenType::LessThan:
                return "<";
            case TokenType::GreaterThan:
                return ">";
            case TokenType::LessEqual:
                return "<=";
            case TokenType::GreaterEqual:
                return ">=";
            case TokenType::RightArrow:
                return "->";
            case TokenType::LeftParen:
                return "(";
            case TokenType::RightParen:
                return ")";
            case TokenType::LeftBrace:
                return "{";
            case TokenType::RightBrace:
                return "}";
            case TokenType::Semicolon:
                return ";";
            case TokenType::Colon:
                return ":";
            case TokenType::DoubleColon:
                return "::";
            case TokenType::Comma:
                return ",";
            case TokenType::Dot:
                return ".";
            case TokenType::Equal:
                return "=";
            case TokenType::Ampersand:
                return "&";
            case TokenType::IntegerLiteral:
                return "integer literal";
            case TokenType::StringLiteral:
                return "string literal";
            case TokenType::Identifier:
                return "an identifier";
            case TokenType::Error:
                return "Error";
            case TokenType::EndOfFile:
                return "eof";
        }
        return "forgot to add new token to Token::getName()";
    }

    TokenType Token::getTokenType() const
    {
        return mTokenType;
    }


    SourceLocation Token::getStartLocation() const
    {
        return mStartLoc;
    }

    SourceLocation Token::getEndLocation() const
    {
        return mEndLoc;
    }
}