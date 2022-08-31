#include <codegen/value/global/function.hh>
#include <codegen/value/instruction/alloca.hh>
#include <algorithm>

namespace Codegen
{
    std::vector<AllocaInst*> allocas;
    Module::Module(const std::string& id)
        :_id(id)
    {
    }

    Function::Function(const std::string& name, bool isDecl, Module& module)
        :Global(module), _name(name), _isDecl(isDecl), _returnType(types.at("int8"))
    {
    }

    const std::pair<std::string, Register*> Function::Generate(Register*)
    {
        if(!_isDecl)
        {
            SortAllocas();
            const std::string argRegs[] = { "di", "si", "dx", "cx", "8", "9" };
            std::string result = "\n\t.globl  " + _name;
            result += "\n\t.type   " + _name + ", @function\n";
            result += _name;

            result += ":\n\tpushq %rbp";
            result += "\n\tmovq %rsp, %rbp";

            unsigned int index = 0;
            for(AllocaInst* arg : _args)
                result += "\n\tmovq %r" + argRegs[index++] + ", " + arg->Generate().first;

            for(BasicBlock* basicBlock : _basicBlockList)
            {
                result += basicBlock->Generate().first;
                delete basicBlock;
            }

            result += "\n\tpopq %rbp";
            result += "\n\tret";
            result += "\n\t.size   " + _name + ", .-" + _name;

            return std::make_pair(result, nullptr);
        }
        return std::make_pair("", nullptr);
    }

    Function* Function::Create(const std::string& name, bool isDecl, Module& module)
    {
        Function* function = new Function(name, isDecl, module);

        module.GetGlobals().push_back(static_cast<Global*>(function));

        return function;
    }

    std::vector<BasicBlock*>& Function::GetBasicBlockList()
    {
        return _basicBlockList;
    }

    std::vector<AllocaInst*>& Function::GetArgList()
    {
        return _args;
    }

    std::string Function::GetName() const
    {
        return _name;
    }

    Type* Function::GetReturnType() const
    {
        return _returnType;
    }

    bool Function::IsDecl() const
    {
        return _isDecl;
    }
    

    void Function::AddAlloca(AllocaInst* alloca)
    {
        allocas.push_back(alloca);
    }

    void Function::SortAllocas()
    {
        std::sort(allocas.begin(), allocas.end(), [](AllocaInst* lhs, AllocaInst* rhs) {
            return lhs->GetType()->GetSize() > rhs->GetType()->GetSize();
        });
        int offset = 0;
        for(AllocaInst* alloca : allocas)
        {
            offset -= alloca->GetType()->GetSize() / 8;
            alloca->_offset = offset;
        }
    }
}