// Copyright 2025 solar-mist

#include "parser/ast/global/ClassDeclaration.h"

#include "type/StructType.h"
#include "type/PointerType.h"

#include "symbol/Mangle.h"

#include <vipir/IR/Function.h>
#include <vipir/Type/FunctionType.h>

namespace parser
{
    ClassField::ClassField(bool priv, Type* type, std::string name)
        : priv(priv)
        , type(type)
        , name(std::move(name))
    {
    }
    
    
    ClassDeclaration::ClassDeclaration(bool exported, bool pending, std::string name, std::vector<ClassField> fields, std::vector<ClassMethod> methods, ScopePtr ownScope, lexer::Token token)
        : ASTNode(ownScope->parent, nullptr, token)
        , mName(std::move(name))
        , mFields(std::move(fields))
        , mMethods(std::move(methods))
        , mOwnScope(std::move(ownScope))
    {
        mSymbolId = mScope->symbols.emplace_back(mName, mType, mScope).id;
        mScope->getSymbol(mSymbolId)->exported = exported;

        std::vector<StructType::Field> structTypeFields;
        std::vector<StructType::Method> structTypeMethods;
        for (auto& field : mFields)
        {
            structTypeFields.push_back(StructType::Field{field.priv, field.name, field.type});
        }
        for (auto& method : mMethods)
        {
            structTypeMethods.push_back(StructType::Method{method.priv, method.name, method.type});
        }

        auto namespaces = mScope->getNamespaces();
        namespaces.push_back(mName);
        auto mangled = StructType::MangleName(namespaces);
        
        Type* thisType = nullptr;
        // We have a pending type on this class
        if (auto type = Type::Get(mangled))
        {
            thisType = type;
            auto pendingType = dynamic_cast<PendingStructType*>(type);
            // TODO: Make sure pendingType exists

            pendingType->set(std::move(structTypeFields), std::move(structTypeMethods));
        }
        else
        {
            if (pending)
            {
                thisType = PendingStructType::Create(mErrorToken, mangled, std::move(structTypeFields), std::move(structTypeMethods));
            }
            else
                thisType = StructType::Create(mangled, std::move(structTypeFields), std::move(structTypeMethods));
        }

        Type* thisPtrType = PointerType::Get(thisType);

        for (auto& method : mMethods)
        {
            auto argumentTypes = method.type->getArgumentTypes();
            argumentTypes.insert(argumentTypes.begin(), thisPtrType);
            method.type = FunctionType::Create(method.type->getReturnType(), std::move(argumentTypes));

            auto scope = method.ownScope->parent;
            method.symbolId = scope->symbols.emplace_back(method.name, method.type, scope).id;
            scope->getSymbol(method.symbolId)->pure = method.pure;
            scope->getSymbol(method.symbolId)->exported = exported;

            method.arguments.insert(method.arguments.begin(), FunctionArgument(thisPtrType, "this"));
            for (auto& argument : method.arguments)
            {
                method.ownScope->symbols.emplace_back(argument.name, argument.type, method.ownScope.get());
            }
            method.ownScope->isPureScope = method.pure;
            method.ownScope->owner = static_cast<StructType*>(thisType);
        }
    }

    std::vector<ASTNode*> ClassDeclaration::getContained() const
    {
        std::vector<ASTNode*> ret;
        for (auto& method : mMethods)
        {
            for (auto& node : method.body)
            {
                ret.push_back(node.get());
            }
        }
        return ret;
    }

    ASTNodePtr ClassDeclaration::clone(Scope* in)
    {
        auto classScope = mOwnScope->clone(in);

        std::vector<ClassMethod> methods;
        for (auto& method : mMethods)
        {
            auto scope = method.ownScope->clone(classScope.get());

            std::vector<ASTNodePtr> bodyClone;
            bodyClone.reserve(method.body.size());
            for (auto& node : method.body)
            {
                bodyClone.push_back(node->clone(scope.get()));
            }
            methods.push_back(ClassMethod{
                method.priv, method.pure, method.name,
                method.type, method.arguments, std::move(bodyClone),
                std::move(scope), method.errorToken
            });
        }
        return std::make_unique<ClassDeclaration>(false, false, mName, mFields, std::move(methods), std::move(classScope), mErrorToken);
    }

    Symbol* ClassDeclaration::getSymbol()
    {
        return mScope->getSymbol(mSymbolId);
    }

    vipir::Value* ClassDeclaration::codegen(vipir::IRBuilder& builder, vipir::Module& module, diagnostic::Diagnostics& diag)
    {
        for (auto& method : mMethods)
        {
            auto names = mScope->getNamespaces();
            names.push_back(mName);
            names.push_back(method.name);
            auto mangledName = mangle::MangleFunction(names, static_cast<FunctionType*>(method.type));

            auto functionType = static_cast<vipir::FunctionType*>(method.type->getVipirType());
            auto function = vipir::Function::Create(functionType, module, mangledName, method.pure);
        
            auto sym = method.ownScope->parent->getSymbol(method.symbolId);
            sym->values.push_back(std::make_pair(nullptr, function));

            if (method.body.empty()) // Probably imported method
            {
                continue;
            }

            auto entryBB = vipir::BasicBlock::Create("", function);
            builder.setInsertPoint(entryBB);

            unsigned int index = 0;
            for (auto& argument : method.arguments)
            {
                auto arg = function->getArgument(index);
                method.ownScope->resolveSymbol(argument.name)->values.push_back(std::make_pair(entryBB, arg));
            }

            for (auto& node : method.body)
            {
                node->codegen(builder, module, diag);
            }
        }

        return nullptr;
    }

    void ClassDeclaration::semanticCheck(diagnostic::Diagnostics& diag, bool& exit, bool statement)
    {
        for (auto& method : mMethods)
        {
            for (auto& node : method.body)
            {
                node->semanticCheck(diag, exit, true);
            }
        }
    }
    
    void ClassDeclaration::typeCheck(diagnostic::Diagnostics& diag, bool& exit)
    {
        for (auto& method : mMethods)
        {
            for (auto& node : method.body)
            {
                node->typeCheck(diag, exit);
            }
        }
    }

    bool ClassDeclaration::triviallyImplicitCast(diagnostic::Diagnostics&, Type*)
    {
        return false;
    }
}