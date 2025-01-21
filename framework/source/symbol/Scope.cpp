// Copyright 2024 solar-mist

#include "symbol/Scope.h"

#include <algorithm>

static unsigned long nextSymbolId = 0;
Symbol::Symbol(std::string name, Type* type)
    : name(name)
    , type(type)
    , id(nextSymbolId++)
{
}

vipir::Value* Symbol::getLatestValue(vipir::BasicBlock* basicBlock)
{
    if (!basicBlock)
    {
        return values.back().second;
    }

    auto it = std::find_if(values.rbegin(), values.rend(), [basicBlock](const auto& value){
        return value.first == basicBlock;
    });
    if (it != values.rend()) return it->second;

    for (auto predecessor : basicBlock->predecessors())
    {
        if (auto value = getLatestValue(predecessor)) return value;
    }
    
    return nullptr;
}

Scope::Scope(Scope* parent, std::string namespaceName, bool isGlobalScope, Type* currentReturnType)
    : parent(parent)
    , namespaceName(std::move(namespaceName))
    , isGlobalScope(isGlobalScope)
    , currentReturnType(currentReturnType)
{
    if (parent && isGlobalScope) parent->children.push_back(this);
}

Scope* Scope::GetGlobalScope()
{
    static Scope globalScope(nullptr, "", true);
    return &globalScope;
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
    std::reverse(namespaces.begin(), namespaces.end());
    return namespaces;
}

Symbol* Scope::getSymbol(unsigned long id)
{
    auto it = std::find_if(symbols.begin(), symbols.end(), [id](const auto& symbol){
        return symbol.id == id;
    });

    if (it != symbols.end()) return &*it;
    return nullptr;
}

Symbol* Scope::resolveSymbol(std::string name)
{
    Scope* current = this;
    while (current)
    {
        auto it = std::find_if(current->symbols.begin(), current->symbols.end(), [&name](const auto& symbol){
            return symbol.name == name;
        });

        if (it != current->symbols.end()) return &*it;
        current = current->parent;
    }
    
    // Scan all scopes if we can't find the symbol by walking up
    if (auto sym = GetGlobalScope()->resolveSymbolDown(name)) return sym;
    return nullptr;
}

Symbol* Scope::resolveSymbol(std::vector<std::string> givenNames)
{
    std::vector<std::string> activeNames = getNamespaces();
    do {
        if (auto sym = GetGlobalScope()->resolveSymbolDown(givenNames)) return sym;

        if (activeNames.empty()) break;
        givenNames.insert(givenNames.begin(), std::move(activeNames.back()));
        activeNames.erase(activeNames.end()-1);
    }
    while (!activeNames.empty());
    return nullptr;
}

Symbol* Scope::resolveSymbolDown(std::string name)
{
    auto it = std::find_if(symbols.begin(), symbols.end(), [&name](const auto& symbol){
        return symbol.name == name;
    });
    if (it != symbols.end()) return &*it;

    for (auto child : children)
    {
        if (auto sym = child->resolveSymbolDown(name))
        {
            return sym;
        }
    }
    return nullptr;
}

Symbol* Scope::resolveSymbolDown(std::vector<std::string> names)
{
    auto namespaces = getNamespaces();
    if (std::equal(namespaces.begin(), namespaces.end(), names.begin(), names.end() - 1))
    {
        // We are in the correct namespace
        auto it = std::find_if(symbols.begin(), symbols.end(), [&names](const auto& symbol){
            return symbol.name == names.back();
        });
        if (it != symbols.end()) return &*it;
    }

    for (auto child : children)
    {
        if (auto sym = child->resolveSymbolDown(names))
        {
            return sym;
        }
    }
    return nullptr;
}

std::vector<Symbol*> Scope::getCandidateFunctions(std::vector<std::string> givenNames)
{
    std::vector<std::string> activeNames = getNamespaces();
    std::vector<Symbol*> candidateFunctions;
    do {
        auto candidates = GetGlobalScope()->getCandidateFunctionsDown(givenNames);
        std::copy(candidates.begin(), candidates.end(), std::back_inserter(candidateFunctions));

        if (activeNames.empty()) break;
        givenNames.insert(givenNames.begin(), std::move(activeNames.back()));
        activeNames.erase(activeNames.end()-1);
    }
    while (!activeNames.empty());

    return candidateFunctions;
}

std::vector<Symbol*> Scope::getCandidateFunctionsDown(std::string name)
{
    std::vector<Symbol*> candidateFunctions;
    auto it = std::find_if(symbols.begin(), symbols.end(), [&name](const auto& symbol){
        return symbol.name == name;
    });

    while (it != symbols.end())
    {
        candidateFunctions.push_back(&*it);
        it = std::find_if(it+1, symbols.end(), [&name](const auto& symbol){
            return symbol.name == name;
        });
    }

    for (auto child : children)
    {
        auto childCandidateFunctions = child->getCandidateFunctionsDown(name);
        std::copy(childCandidateFunctions.begin(), childCandidateFunctions.end(), std::back_inserter(candidateFunctions));
    }
    return candidateFunctions;
}

std::vector<Symbol*> Scope::getCandidateFunctionsDown(std::vector<std::string> names)
{
    std::vector<Symbol*> candidateFunctions;
    auto namespaces = getNamespaces();
    if (std::equal(namespaces.begin(), namespaces.end(), names.begin(), names.end() - 1))
    {
        // We are in the correct namespace
        auto it = std::find_if(symbols.begin(), symbols.end(), [&names](const auto& symbol){
            return symbol.name == names.back();
        });
        while (it != symbols.end())
        {
            candidateFunctions.push_back(&*it);
            it = std::find_if(it+1, symbols.end(), [&names](const auto& symbol){
                return symbol.name == names.back();
            });
        }
    }

    for (auto child : children)
    {
        auto childCandidateFunctions = child->getCandidateFunctionsDown(names);
        std::copy(childCandidateFunctions.begin(), childCandidateFunctions.end(), std::back_inserter(candidateFunctions));
    }
    return candidateFunctions;
}