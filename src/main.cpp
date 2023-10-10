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

std::int32_t test[100] = {
        43,69,87,95,31,91,72,9,4,83,62,36,75,71,42,61,33,3,51,47,5,27,29,34,
        56,96,97,82,11,74,17,46,52,14,40,2,50,78,7,65,86,35,94,12,66,79,21,
        18,59,67,19,53,58,25,28,76,70,26,92,1,37,23,90,88,89,15,99,16,77,45,
        81,63,54,13,10,41,55,30,68,84,98,6,22,32,49,57,73,60,85,48,44,100,38,93,64,8,39,80,20,24
};

int main(int argc, const char** argv)
{
    blt::arg_parse parser;
    
    parser.addArgument(blt::arg_builder("--capacity", "-c").setAction(blt::arg_action_t::STORE).setNArgs(1)
                                                           .setHelp("Set the number of customers. (Default: 200)")
                                                           .setDefault("200").build());
    parser.addArgument(blt::arg_builder("--problemset", "-p").setAction(blt::arg_action_t::STORE).setNArgs(1)
                                                             .setHelp("Set where to load the problem set from, defaults to r101")
                                                             .setDefault("../problems/r101.set").build());
    
    auto args = parser.parse_args(argc, argv);
    
    auto loaded_problems = load_problem(args.get<std::string>("problemset"));
    
    ga::init(args.get<int32_t>("capacity"), std::move(loaded_problems));
    
    ga::chromosome ch{};
    std::memcpy(ch.genes, test, 100 * sizeof(std::int32_t));
    
    auto routes = ga::constructRoute(ch);
    int total = 0;
    for (int i = 0; i < routes.size(); i++){
        std::string route;
        for (const auto& v : routes[i].customers)
        {
            route += std::to_string(v);
            route += " ";
            total++;
        }
        BLT_TRACE("route %d with '%s'", i+1, route.c_str());
    }
    BLT_DEBUG("Total: %d", total);
    
    auto ret = ga::execute();
    
    ga::destroy();
    
    return ret;
}
