// Copyright 2025 solar-mist

#include "symbol/ImportManager.h"

#include "diagnostic/Diagnostic.h"

#include "lexer/Lexer.h"
#include "lexer/Token.h"

#include "parser/SymbolParser.h"

#include <fstream>

ImportManager::ImportManager()
    : mSearchPaths{std::filesystem::current_path()}
{
}

std::vector<parser::ASTNodePtr> ImportManager::resolveImports(std::filesystem::path path, std::filesystem::path relativeTo, Scope* scope)
{
    path += ".vpr";

    std::ifstream stream;
    stream.open(relativeTo.parent_path() / path);
    std::string foundPath;
    if (!stream.is_open())
    {
        for (auto& searchPath : mSearchPaths)
        {
            stream.open(searchPath / path);
            if (stream.is_open())
            {
                foundPath = searchPath;
                break;
            }
        }
    }
    else
    {
        foundPath = relativeTo.parent_path() / path;
    }

    mImportedFiles.push_back(foundPath);

    diagnostic::Diagnostics importerDiag;

    std::stringstream buf;
    buf << stream.rdbuf();
    std::string text = buf.str();

    importerDiag.setText(text);
    importerDiag.setImported(true);

    lexer::Lexer lexer(text, mImportedFiles.back());
    auto tokens = lexer.lex();

    parser::SymbolParser parser(tokens, importerDiag, *this, scope);
    
    return parser.parse();
}

void ImportManager::seizeScope(ScopePtr scope)
{
    mScopes.push_back(std::move(scope));
}