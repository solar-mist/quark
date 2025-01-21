// Copyright 2025 solar-mist

#include "symbol/Identifier.h"

namespace symbol
{
    QualifiedIdentifier::QualifiedIdentifier(std::vector<std::string> identifiers)
        : mIdentifiers(std::move(identifiers))
    {
    }

    std::vector<std::string>& QualifiedIdentifier::getIdentifiers()
    {
        return mIdentifiers;
    }
}