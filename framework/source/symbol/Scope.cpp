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
    // TODO: Namespace lookups
    Scope* current = this;
    while (current)
    {
        auto it = std::find_if(current->symbols.begin(), current->symbols.end(), [&name](const auto& symbol){
            return symbol.name == name;
        });

        if (it != current->symbols.end()) return &*it;

        for (auto& scope : current->importedScopes)
        {
            if (auto sym = scope->resolveSymbol(name))
            {
                if (sym->exported) return sym;
            }
        }
        current = current->parent;
    }
    return nullptr;
}

std::vector<Symbol*> Scope::getCandidateFunctions(std::string name)
{
    // TODO: Namespace lookups
    std::vector<Symbol*> candidateFunctions;
    Scope* current = this;
    std::vector<Symbol*>::iterator prev;
    while (current)
    {
        auto it = std::find_if(current->symbols.begin(), current->symbols.end(), [&name](const auto& symbol){
            return symbol.name == name;
        });

        while (it != current->symbols.end())
        {
            candidateFunctions.push_back(&*it);
            it = std::find_if(it+1, current->symbols.end(), [&name](const auto& symbol){
                return symbol.name == name;
            });
        }

        for (auto& scope : current->importedScopes)
        {
            auto candidates = scope->getCandidateFunctions(name);
            std::erase_if(candidates, [](Symbol* symbol) {
                return !symbol->exported;
            });

            std::copy(candidates.begin(), candidates.end(), std::back_inserter(candidateFunctions));
        }
        current = current->parent;
    }
    return candidateFunctions;
}