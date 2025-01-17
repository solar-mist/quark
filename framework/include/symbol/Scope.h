// Copyright 2024 solar-mist

#ifndef VIPER_FRAMEWORK_SYMBOL_SCOPE_H
#define VIPER_FRAMEWORK_SYMBOL_SCOPE_H 1

#include "type/Type.h"

#include <vipir/IR/Value.h>
#include <vipir/IR/BasicBlock.h>

#include <memory>
#include <string>
#include <vector>

struct Scope;

struct Symbol
{
    Symbol(std::string name, Type* type);

    vipir::Value* getLatestValue(vipir::BasicBlock* basicBlock = nullptr);
    vipir::Value* getLatestValue(Scope* scope, vipir::BasicBlock* basicBlock = nullptr);

    std::string name;
    Type* type;
    std::vector<std::pair<vipir::BasicBlock*, vipir::Value*> > values;
    unsigned long id;
    bool pure{ false };
    bool exported;
};

struct Scope
{
    Scope(Scope* parent, std::string namespaceName, bool isGlobalScope, Type* currentReturnType = nullptr);

    std::vector<std::string> getNamespaces();

    Symbol* getSymbol(unsigned long id);
    Symbol* resolveSymbol(std::string name);
    std::vector<Symbol*> getCandidateFunctions(std::string name);

    Scope* parent;
    
    std::string namespaceName;
    bool isGlobalScope;
    bool isPureScope;

    Type* currentReturnType;
    std::vector<Symbol> symbols;

    std::vector<std::unique_ptr<Scope> > importedScopes;
};
using ScopePtr = std::unique_ptr<Scope>;

#endif // VIPER_FRAMEWORK_SYMBOL_SCOPE_H