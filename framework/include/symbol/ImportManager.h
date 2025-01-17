// Copyright 2025 solar-mist

#ifndef VIPER_FRAMEWORK_SYMBOL_IMPORT_MANAGER_H
#define VIPER_FRAMEWORK_SYMBOL_IMPORT_MANAGER_H 1

#include "symbol/Scope.h"

#include "parser/ast/ASTNode.h"

#include <filesystem>
#include <string>
#include <vector>

class ImportManager
{
public:
    ImportManager();

    std::vector<parser::ASTNodePtr> resolveImports(std::filesystem::path path, Scope* scope);

private:
    std::vector<std::string> mSearchPaths;
};

#endif // VIPER_FRAMEWORK_SYMBOL_IMPORT_MANAGER_H