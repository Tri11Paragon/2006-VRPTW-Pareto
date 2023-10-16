#include <iostream>

#include <program.h>
#include <blt/parse/argparse.h>
#include <blt/std/logging.h>
#include <blt/std/system.h>
//#include <blt/profiling/profiler.h>
#include <blt/profiling/profiler_v2.h>
#include <any>
#include <loader.h>
#include <blt/std/string.h>

int main(int argc, const char** argv)
{
    blt::arg_parse parser;
    
    parser.addArgument(blt::arg_builder("--capacity", "-c").setAction(blt::arg_action_t::STORE).setNArgs(1)
                                                           .setHelp("Set the capacity of the trucks. (Default: 200)")
                                                           .setDefault("200").build());
    parser.addArgument(blt::arg_builder("--problemset", "-p").setAction(blt::arg_action_t::STORE).setNArgs(1)
                                                             .setHelp("Set where to load the problem set from, defaults to r101")
                                                             .setDefault("../problems/c101.set").build());
    
    auto args = parser.parse_args(argc, argv);
    
    auto loaded_problems = load_problem(args.get<std::string>("problemset"));
    
    ga::program p(args.get<int32_t>("capacity"), std::move(loaded_problems));
    
    std::int32_t skip = 0;
    
    std::string whatToDo;
    
    while (true)
    {
        while (skip-- > 0)
        {
            p.executeStep();
        }
        BLT_INFO("What do we do(%d/%d)? ", p.steps(), p.GENERATION_COUNT);
        std::getline(std::cin, whatToDo);
        if (whatToDo.empty())
            continue;
        if (blt::string::is_numeric(whatToDo))
        {
            skip = std::stoi(whatToDo);
        } else
        {
            whatToDo = blt::string::toLowerCase(whatToDo);
            if (whatToDo == "exit" || blt::string::contains(whatToDo, 'q'))
                return 0;
            if (whatToDo == "print")
                p.print();
            else if (blt::string::contains(whatToDo, "val"))
                p.validate();
            else if (blt::string::contains(whatToDo, 'w'))
                p.write(whatToDo);
            else if (blt::string::contains(whatToDo, 'h')){
                p.writeHistory();
            } else
            {
                BLT_INFO("Not a command.");
                continue;
            }
        }
    }
    
}
