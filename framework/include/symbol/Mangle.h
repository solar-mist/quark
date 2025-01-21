// Copyright 2024 solar-mist

#ifndef VIPER_FRAMEWORK_SYMBOL_MANGLE_H
#define VIPER_FRAMEWORK_SYMBOL_MANGLE_H 1

#include <type/FunctionType.h>

#include <string>

namespace mangle
{
    std::string MangleFunction(std::string_view name, FunctionType* type);
    std::string MangleFunction(const std::vector<std::string>& names, FunctionType* type);
}

#endif // VIPER_FRAMEWORK_SYMBOL_MANGLE_H