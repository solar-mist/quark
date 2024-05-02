// Copyright 2024 solar-mist


#include "preprocessor/Preprocessor.h"

#include "lexer/Lexer.h"
#include "lexer/Token.h"

#include "parser/Parser.h"

#include "type/Type.h"

#include <vipir/IR/IRBuilder.h>
#include <vipir/Module.h>
#include <vipir/ABI/SysV.h>

#include <fstream>
#include <iostream>
#include <sstream>

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "viper: no input files";
        std::exit(1);
    }

    std::ifstream file = std::ifstream(argv[1]);

    std::stringstream buffer;
    buffer << file.rdbuf();

    Type::Init();

    preprocessor::Preprocessor preprocessor(buffer.str());
    preprocessor.preprocess();

    lexing::Lexer lexer(preprocessor.getText());

    std::vector<lexing::Token> tokens = lexer.lex();

    parser::Parser parser(tokens);
    
    vipir::IRBuilder builder;
    vipir::Module module(argv[1]);
    module.setABI<vipir::abi::SysV>();

    for (auto& node : parser.parse())
    {
        node->emit(builder, module, nullptr);
    }

    module.print(std::cout);

    using namespace std::literals;
    std::ofstream outfile = std::ofstream(argv[1] + ".o"s);
    module.emit(outfile, vipir::OutputFormat::ELF);

    return 0;
}