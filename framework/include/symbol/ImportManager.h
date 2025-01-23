// Copyright 2025 solar-mist

#ifndef VIPER_FRAMEWORK_SYMBOL_IMPORT_MANAGER_H
#define VIPER_FRAMEWORK_SYMBOL_IMPORT_MANAGER_H 1

#include "symbol/Scope.h"

#include "parser/ast/ASTNode.h"

#include <filesystem>
#include <string>
#include <vector>

struct Export;

class ImportManager
{
public:
    ImportManager();

    std::vector<Export> getExports(std::string file, Scope* scope);
    std::vector<std::string> getPendingStructTypeNames();
    void clearExports();
    void addPendingStructType(std::string name);
    bool wasExportedTo(std::string root, std::vector<Export>& exports, Export& exp);

    std::vector<parser::ASTNodePtr> resolveImports(std::filesystem::path path, std::filesystem::path relativeTo, Scope* scope, bool exported);

    void seizeScope(ScopePtr scope);

private:
    std::vector<std::string> mSearchPaths;
    std::vector<std::string> mImportedFiles;
    std::vector<ScopePtr> mScopes;

    std::vector<Export> mExports;
    std::vector<std::string> mPendingStructTypeNames;
};

struct Export
{
    std::string exportedFrom;
    Symbol* symbol;
    std::string exportedTo;
};

#endif // VIPER_FRAMEWORK_SYMBOL_IMPORT_MANAGER_H