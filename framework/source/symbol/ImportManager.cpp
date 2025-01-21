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

std::vector<parser::ASTNodePtr> ImportManager::resolveImports(std::filesystem::path path, Scope* scope)
{
    path += ".vpr";

    std::ifstream stream;
    for (auto searchPath : mSearchPaths)
    {
        stream.open(searchPath / path);
        if (stream.is_open()) break;
    }

    mImportedFiles.push_back(path.string());

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