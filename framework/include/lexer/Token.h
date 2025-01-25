// Copyright 2024 solar-mist

#ifndef VIPER_FRAMEWORK_LEXER_TOKEN_H
#define VIPER_FRAMEWORK_LEXER_TOKEN_H 1

#include "lexer/SourceLocation.h"

#include <string>

namespace lexer
{
    enum class TokenType
    {
        // Keywords
        FuncKeyword,
        ReturnKeyword,
        TypeKeyword,
        LetKeyword,
        IfKeyword,
        ElseKeyword,
        TrueKeyword,
        FalseKeyword,
        PureKeyword,
        ExportKeyword,
        ImportKeyword,
        ClassKeyword,
        PublicKeyword,
        NamespaceKeyword,

        // Symbols
        Plus,
        Minus,
        Star,
        Slash,

        DoubleEqual,
        BangEqual,
        LessThan,
        GreaterThan,
        LessEqual,
        GreaterEqual,

        RightArrow,

        LeftParen,
        RightParen,

        LeftBrace,
        RightBrace,

        Semicolon,
        Colon,
        DoubleColon,
        Comma,
        Dot,

        Ampersand,

        Equal,

        // Literals
        IntegerLiteral,
        StringLiteral,

        // Misc
        Identifier,
        Error,
        EndOfFile,
    };

    class Token
    {
    public:
        Token(std::string text, TokenType tokenType, SourceLocation startLocation, SourceLocation endLocation);

        std::string_view getText() const;
        std::string_view getName() const; // For diagnostics
        TokenType getTokenType() const;

        SourceLocation getStartLocation() const;
        SourceLocation getEndLocation() const;

    private:
        std::string mText;
        TokenType mTokenType;

        SourceLocation mStartLoc;
        SourceLocation mEndLoc;
    };
}

#endif // VIPER_FRAMEWORK_LEXER_TOKEN_H