// Copyright 2025 solar-mist

#include "symbol/ImportManager.h"

#include "diagnostic/Diagnostic.h"

#include "lexer/Lexer.h"
#include "lexer/Token.h"

#include "parser/Parser.h"

#include "type/StructType.h"

#include <fstream>

ImportManager::ImportManager()
    : mSearchPaths{std::filesystem::current_path()}
{
}

std::vector<Export> ImportManager::getExports()
{
    return mExports;
}

std::vector<std::vector<std::string> > ImportManager::getPendingStructTypeNames()
{
    return mPendingStructTypeNames;
}

void ImportManager::clearExports()
{
    mExports.clear();
    mPendingStructTypeNames.clear();
}

void ImportManager::addPendingStructType(std::vector<std::string> names)
{
    mPendingStructTypeNames.push_back(std::move(names));
}

bool ImportManager::wasExportedTo(std::string root, std::vector<Import>& imports, Export& exp)
{
    if (exp.symbol && !exp.symbol->exported) return false;
    std::function<bool(std::string)> checkOne;
    checkOne = [&](std::string path) {
        if (path == root) return true;

        auto it = std::find_if(imports.begin(), imports.end(), [&](auto& exp) {
            return exp.from == path && !exp.to.empty();
        });
        if (it != imports.end()) return checkOne(it->to);
        return false;
    };
    return checkOne(exp.exportedFrom);
}

void ImportManager::collectAllImports(std::filesystem::path path, std::filesystem::path relativeTo, std::vector<Import>& imports)
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

    auto it = std::find_if(imports.begin(), imports.end(), [&foundPath](auto& import){
        return import.from == foundPath;
    });
    bool exists = it != imports.end();

    imports.push_back({foundPath, relativeTo});

    if (exists) return;
    
    diagnostic::Diagnostics importerDiag;

    std::stringstream buf;
    buf << stream.rdbuf();
    std::string text = buf.str();

    importerDiag.setText(text);
    importerDiag.setImported(true);

    lexer::Lexer lexer(text, foundPath);
    auto tokens = lexer.lex();

    parser::Parser parser(tokens, importerDiag, *this, nullptr);
    for (auto& import : parser.findImports())
    {
        collectAllImports(import, foundPath, imports);
    }
}

std::vector<parser::ASTNodePtr> ImportManager::resolveImports(std::filesystem::path path, std::filesystem::path relativeTo, Scope* scope, bool exported)
{
    std::string foundPath = path.string();

    std::ifstream stream;
    stream.open(path);

    mImportedFiles.push_back(foundPath);

    diagnostic::Diagnostics importerDiag;

    std::stringstream buf;
    buf << stream.rdbuf();
    std::string text = buf.str();

    importerDiag.setText(text);
    importerDiag.setImported(true);

    lexer::Lexer lexer(text, mImportedFiles.back());
    auto tokens = lexer.lex();

    parser::Parser parser(tokens, importerDiag, *this, scope, true);
    auto ast = parser.parse();

    // Add an export if this was an export import
    if (exported)
        mExports.push_back({path, nullptr, relativeTo.string()});
    
    std::function<std::vector<Export>(Scope*)> collectScope;
    collectScope = [&path, &collectScope, &foundPath](Scope* scope) {
        std::vector<Export> ret;
        for (auto& symbol : scope->symbols)
        {
            ret.push_back({foundPath, &symbol});
        }
        for (auto child : scope->children)
        {
            auto childExports = collectScope(child);
            std::copy(childExports.begin(), childExports.end(), std::back_inserter(ret));
        }
        return ret;
    };
    auto exports = collectScope(scope);
    std::copy(exports.begin(), exports.end(), std::back_inserter(mExports));
    
    return ast;
}

void ImportManager::reportUnknownTypeErrors()
{
    auto pendings = PendingStructType::GetPending();

    for (auto pending : pendings)
    {
        std::filesystem::path file = pending->getToken().getStartLocation().file;
        std::ifstream stream;

        stream.open(file); // TODO: Store files and then clear them after this function?
        std::stringstream buf;
        buf << stream.rdbuf();
        std::string text = buf.str();

        diagnostic::Diagnostics diag;
        diag.setText(text);
        diag.setImported(true);

        diag.reportCompilerError(pending->getToken().getStartLocation(), pending->getToken().getEndLocation(), std::format("unknown type name '{}{}{}'",
            fmt::bold, pending->getName(), fmt::defaults
        ));
    }
    if (pendings.size()) std::exit(1);
}

void ImportManager::seizeScope(ScopePtr scope)
{
    mScopes.push_back(std::move(scope));
}