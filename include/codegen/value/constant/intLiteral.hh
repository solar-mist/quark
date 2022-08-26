#ifndef VIPER_CODEGEN_INTLITERAL_HH
#define VIPER_CODEGEN_INTLITERAL_HH
#include <codegen/value/value.hh>

namespace Codegen
{
    class IntegerLiteral : public Value
    {
    public:
        IntegerLiteral(const long long value, const Module& module);
        const std::string Generate() override;
    private:
        long long _value;
    };
}

#endif