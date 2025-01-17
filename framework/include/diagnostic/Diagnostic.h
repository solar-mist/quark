// Copyright 2024 solar-mist

#ifndef VIPER_FRAMEWORK_DIAGNOSTIC_DIAGNOSTIC_H
#define VIPER_FRAMEWORK_DIAGNOSTIC_DIAGNOSTIC_H 1

#include <string>
#include <vector>

namespace lexer
{
    class SourceLocation;
}

namespace fmt
{
    constexpr std::string_view bold     = "\x1b[1m";
    constexpr std::string_view red      = "\x1b[31m";
    constexpr std::string_view yellow   = "\x1b[93m";
    constexpr std::string_view defaults = "\x1b[0m";
}

namespace diagnostic
{
    class Diagnostics
    {
    public:
        Diagnostics();

        void setText(std::string_view text);
        void setWarning(bool enable, std::string_view warning);
        void setImported(bool imported);

        [[noreturn]] void fatalError(std::string_view message);

        void reportCompilerError(lexer::SourceLocation start, lexer::SourceLocation end, std::string_view message);
        void compilerWarning(std::string_view type, lexer::SourceLocation start, lexer::SourceLocation end, std::string_view message);

    private:
        std::string_view mText;
        std::vector<std::string_view> mWarnings;
        bool mImported{ false };

        int getLinePosition(int lineNumber);
    };
}

#endif // VIPER_FRAMEWORK_DIAGNOSTIC_DIAGNOSTIC_H