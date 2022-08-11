#include <environment.hxx>

namespace Viper
{
    llvm::AllocaInst* FindNamedValue(std::string name, std::shared_ptr<Environment> scope)
    {
        while(true)
        {
            if(scope->namedValues.find(name) != scope->namedValues.end())
                return scope->namedValues[name];
            else if(scope->outer)
                scope = scope->outer;
            else
                throw; // TODO: Error properly
        }
    }
}