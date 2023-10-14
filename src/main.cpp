#include <iostream>

#include <program.h>
#include <blt/parse/argparse.h>
#include <blt/std/logging.h>
#include <blt/std/system.h>
//#include <blt/profiling/profiler.h>
#include <blt/profiling/profiler_v2.h>

#include <algorithm>
#include <numeric>
#include <vector>
#include <random>
#include <functional>
#include <any>
#include <loader.h>

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
    
    ga::init(args.get<int32_t>("capacity"), std::move(loaded_problems));
    
    auto ret = ga::execute();
    
    ga::destroy();
    
    return ret;
}
