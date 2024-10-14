// Copyright 2024 solar-mist

#include "symbol/Scope.h"

#include <algorithm>

Symbol::Symbol(std::string name, Type* type)
    : name(name)
    , type(type)
    , value(nullptr)
{
}

Scope::Scope(Scope* parent, std::string namespaceName, bool isGlobalScope, Type* currentReturnType)
    : parent(parent)
    , namespaceName(std::move(namespaceName))
    , isGlobalScope(isGlobalScope)
    , currentReturnType(currentReturnType)
{
}

std::vector<std::string> Scope::getNamespaces()
{
    std::vector<std::string> namespaces;
    Scope* current = this;
    while (current)
    {
        namespaces.push_back(current->namespaceName);
        current = current->parent;
    }
    return namespaces;
}

Symbol* Scope::resolveSymbol(std::string name)
{
    // TODO: Namespace lookups
    Scope* current = this;
    while (current)
    {
        auto it = std::find_if(current->symbols.begin(), current->symbols.end(), [&name](const auto& symbol){
            return symbol.name == name;
        });

        if (it != current->symbols.end()) return &*it;
        current = current->parent;
    }
    return nullptr;
}