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
                                                            .setHelp("Set the number of customers. (Default: 100)")
                                                            .setDefault("200").build());
    
    auto args = parser.parse_args(argc, argv);
    
    auto loaded_problems = load_problem("../problems/r101.set");
    
    BLT_TRACE("%d", loaded_problems.size());
    BLT_TRACE("%d", loaded_problems[0].y);
    
    ga::program p(args.get<int32_t>("capacity"));
    
    
    return p.execute();
}
