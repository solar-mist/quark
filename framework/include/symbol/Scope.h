// Copyright 2024 solar-mist

#ifndef VIPER_FRAMEWORK_SYMBOL_SCOPE_H
#define VIPER_FRAMEWORK_SYMBOL_SCOPE_H 1

#include "type/Type.h"
#include "type/StructType.h"

#include <vipir/IR/Value.h>
#include <vipir/IR/BasicBlock.h>

#include <memory>
#include <string>
#include <vector>

struct Scope;
struct Symbol;
struct Export;
namespace parser { struct ASTNode; using ASTNodePtr = std::unique_ptr<ASTNode>; }

struct TemplateParameter
{
    std::string name;
    Type* type;
};

struct TemplateInstantiation
{
    parser::ASTNodePtr body;
    std::vector<Type*> parameters;
    Export* exp;
};

struct TemplateSymbol
{
    TemplateSymbol(std::vector<TemplateParameter> parameters, parser::ASTNodePtr body, int symbolId, Scope* in);
    std::vector<TemplateParameter> parameters;
    parser::ASTNodePtr body;
    std::vector<TemplateInstantiation> instantiations;
    int symbolId;
    Scope* in;
};

struct Symbol
{
    Symbol(std::string name, Type* type, Scope* owner);

    vipir::Value* getLatestValue(vipir::BasicBlock* basicBlock = nullptr);
    vipir::Value* getLatestValue(Scope* scope, vipir::BasicBlock* basicBlock = nullptr);

    Symbol clone(Scope* in);

    bool removed; // We don't want to actually remove these because there might be references to it

    std::string name;
    Type* type;
    Scope* owner;
    std::vector<std::pair<vipir::BasicBlock*, vipir::Value*> > values;
    unsigned long id;
    bool pure{ false };
    bool exported;

    std::unique_ptr<TemplateSymbol> templated;
};

struct Scope
{
    Scope(Scope* parent, std::string namespaceName, bool isGlobalScope, Type* currentReturnType = nullptr);

    static Scope* GetGlobalScope();

    std::unique_ptr<Scope> clone(Scope* in);

    std::vector<std::string> getNamespaces();

    Type* findOwner();

    Symbol* getSymbol(unsigned long id);
    Symbol* resolveSymbol(std::string name);
    Symbol* resolveSymbol(std::vector<std::string> givenNames);
    std::vector<Symbol*> getCandidateFunctions(std::vector<std::string> givenNames);

    Scope* parent;
    
    std::string namespaceName;
    bool isGlobalScope;
    bool isPureScope;

    Type* currentReturnType;
    Type* owner;

    std::vector<Symbol> symbols;

    std::vector<Scope*> children;

private:
    // Scans all global scopes for a symbol/candidate functions
    Symbol* resolveSymbolDown(std::string name);
    Symbol* resolveSymbolDown(std::vector<std::string> names);
    std::vector<Symbol*> getCandidateFunctionsDown(std::string name);
    std::vector<Symbol*> getCandidateFunctionsDown(std::vector<std::string> names);
};
using ScopePtr = std::unique_ptr<Scope>;

#endif // VIPER_FRAMEWORK_SYMBOL_SCOPE_H