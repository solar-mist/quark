// Copyright 2024 solar-mist

#include "Options.h"

#include "vipir/Optimizer/Peephole/PeepholeV2.h"
#include "vipir/Pass/DefaultPass.h"

#include <algorithm>
#include <array>

std::vector<Option> Option::ParseOptions(int argc, char **argv)
{
    std::vector<Option> options;
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg.starts_with('-'))
        {
            switch (arg[1])
            {
                case 'W':
                    options.push_back({OptionType::WarningSpec, arg.substr(2)});
                    break;

                case 'f':
                    options.push_back({OptionType::FlagSpec, arg.substr(2)});
                    break;

                case 'O':
                    options.push_back({OptionType::OptimizationLevelSpec, arg.substr(2)});
                    break;
            }
        }
        else
        {
            options.push_back({OptionType::InputFile, arg});
        }
    }
    return options;
}

std::string Option::GetInputFile(const std::vector<Option>& options)
{
    auto it = std::find_if(options.begin(), options.end(), [](const auto& option){
        return option.type == OptionType::InputFile;
    });
    if (it != options.end()) return it->value;
    return "";
}

struct FlagPass
{
    std::string_view id;
    vipir::PassType insertBefore;
    std::unique_ptr<vipir::Pass>(*build)();
};

static std::array passes = {
    FlagPass{ "dce", vipir::PassType::LIREmission, []()->std::unique_ptr<vipir::Pass>{ return std::make_unique<vipir::opt::DCEPass>(); } },
    FlagPass{ "dead-code-elimination", vipir::PassType::LIREmission, []()->std::unique_ptr<vipir::Pass>{ return std::make_unique<vipir::opt::DCEPass>(); } },
    FlagPass{ "peephole", vipir::PassType::LIRCodegen, []()->std::unique_ptr<vipir::Pass>{ return std::make_unique<vipir::opt::PeepholePass>(); } },
    FlagPass{ "constant-folding", vipir::PassType::DeadCodeElimination, []()->std::unique_ptr<vipir::Pass>{ return std::make_unique<vipir::ConstantFoldingPass>(); } },
    FlagPass{ "mem2reg", vipir::PassType::DeadCodeElimination, []()->std::unique_ptr<vipir::Pass>{ return std::make_unique<vipir::opt::Mem2RegPass>(); } },
};
static auto FindPass(std::string_view name)
{
    return std::find_if(passes.begin(), passes.end(), [name](const auto& pass){
        return pass.id == name;
    });
}

void Option::ParseOptimizingFlags(const std::vector<Option>& options, vipir::Module& module, diagnostic::Diagnostics& diag) 
{
    for (const auto& option : options)
    {
        if (option.type == OptionType::FlagSpec)
        {
            if (option.value.starts_with("no-"))
            {
                auto it = FindPass(option.value.substr(3));
                if (it != passes.end());
                    // TODO: module.removePass(it->second);
            }
            else
            {
                auto it = FindPass(option.value);
                if (it != passes.end())
                {
                    module.getPassManager().insertBefore(it->insertBefore, it->build());
                }
            }
        }
        else if (option.type == OptionType::OptimizationLevelSpec)
        {
            char level = option.value.empty() ? '1' : option.value[0];

            switch (level)
            {
                case '0':
                case 'g': // debug, shouldn't do anything
                    //TODO: module.removeAllPasses();
                    break;

                case '3':
                case '2':
                case 's': // size, should do everything except size increasing optimizations
                case '1':
                    module.getPassManager().insertBefore(vipir::PassType::LIREmission, std::make_unique<vipir::opt::DCEPass>());
                    module.getPassManager().insertBefore(vipir::PassType::DeadCodeElimination, std::make_unique<vipir::ConstantFoldingPass>());
                    module.getPassManager().insertAfter(vipir::PassType::DeadCodeElimination, std::make_unique<vipir::opt::Mem2RegPass>());
                    module.getPassManager().insertBefore(vipir::PassType::LIRCodegen, std::make_unique<vipir::opt::PeepholePass>());
                    break;

                default:
                    diag.fatalError("argument to -O should be '0', '1', '2', '3', 'g' or 's'");
            }
        }
    }
}