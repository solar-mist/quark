// Copyright 2025 solar-mist

#ifndef VIPER_FRAMEWORK_SYMBOL_IDENTIFIER_H
#define VIPER_FRAMEWORK_SYMBOL_IDENTIFIER_H 1

#include <string>
#include <vector>

namespace symbol
{
    class QualifiedIdentifier
    {
    public:
        QualifiedIdentifier(std::vector<std::string> identifiers);

        std::vector<std::string>& getIdentifiers();

    private:
        std::vector<std::string> mIdentifiers;
    };
}

#endif // VIPER_FRAMEWORK_SYMBOL_IDENTIFIER_H