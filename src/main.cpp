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
#include <blt/window/window.h>
#include <implot.h>
#include "blt/std/assert.h"
#include "blt/std/time.h"

int main(int argc, const char** argv)
{
    blt::arg_parse parser;
    
    parser.addArgument(blt::arg_builder("--capacity", "-c").setAction(blt::arg_action_t::STORE).setNArgs(1)
                                                           .setHelp("Set the capacity of the trucks. (Default: 200)")
                                                           .setDefault("200").build());
    parser.addArgument(blt::arg_builder("--problemset", "-p").setAction(blt::arg_action_t::STORE).setNArgs(1)
                                                             .setHelp("Set where to load the problem set from, defaults to r101")
                                                             .setDefault("../problems/r101.set").build());

#ifdef BLT_BUILD_GLFW
    blt::init_glfw();
#endif
    
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
            else if (blt::string::contains(whatToDo, 'h'))
                p.writeHistory();
            else if (blt::string::contains(whatToDo, 'r'))
                p.reset();
            else if (blt::string::contains(whatToDo, "t"))
            {
                std::vector<std::vector<ga::point>> avg_history;
                std::vector<std::vector<ga::point>> best_history;
                std::vector<ga::point> best_solution;
                std::vector<ga::point> worst_best_solution;
                
                static constexpr std::int32_t GEN_COUNT = 500;
                
                for (int i = 0; i < 25; i++)
                {
                    auto problem = load_problem(args.get<std::string>("problemset"));
                    
                    ga::program cp(args.get<int32_t>("capacity"), std::move(problem));
                    
                    for (int j = 0; j < GEN_COUNT; j++)
                        cp.executeStep();
                    
                    auto b = cp.getBestHistory();
                    auto a = cp.getBestHistory();
                    
                    avg_history.push_back(a);
                    best_history.push_back(b);
                    
                    if (best_solution.empty())
                        best_solution = b;
                    if (worst_best_solution.empty())
                        worst_best_solution = b;
                    
                    auto avgCars = std::numeric_limits<unsigned long>::max();
                    auto avgDist = std::numeric_limits<double>::max();
                    
                    for (auto v : b)
                    {
                        if (v.routes <= avgCars && v.distance <= avgDist)
                        {
                            avgCars = v.routes;
                            avgDist = v.distance;
                        }
                    }
                    
                    auto bAvgCars = std::numeric_limits<unsigned long>::max();
                    auto bAvgDist = std::numeric_limits<double>::max();
                    
                    for (auto v : best_solution)
                    {
                        if (v.routes <= bAvgCars && v.distance <= bAvgDist)
                        {
                            bAvgCars = v.routes;
                            bAvgDist = v.distance;
                        }
                    }
                    
                    if (avgCars <= bAvgCars && avgDist <= bAvgDist)
                        best_solution = b;
                    
                    auto wAvgCars = std::numeric_limits<unsigned long>::max();
                    auto wAvgDist = std::numeric_limits<double>::max();
                    
                    for (auto v : worst_best_solution)
                    {
                        if (v.routes <= wAvgCars && v.distance <= wAvgDist)
                        {
                            wAvgCars = v.routes;
                            wAvgDist = v.distance;
                        }
                    }
                    
                    if (avgCars >= wAvgCars && avgDist >= wAvgDist)
                        worst_best_solution = b;
                }
                
                auto bestCars = std::numeric_limits<unsigned long>::max();
                auto bestDist = std::numeric_limits<double>::max();
                
                auto worstCars = 0ul;
                auto worstDist = 0.0;
                
                for (const auto& v : worst_best_solution)
                {
                    if (v.distance >= worstDist && v.routes >= worstCars)
                    {
                        worstCars = v.routes;
                        worstDist = v.distance;
                    }
                }
                
                BLT_INFO("Over all the worst best solutions are: %f distance %d routes", worstDist, worstCars);
                
                for (const auto& v : best_solution)
                {
                    if (v.distance <= bestDist && v.routes <= bestCars)
                    {
                        bestCars = v.routes;
                        bestDist = v.distance;
                    }
                }
                BLT_INFO("Over all the best solutions are: %f distance %d routes", bestDist, bestCars);
                
                std::string best_file{"./ga25_bests_history_"};
                best_file += blt::system::getTimeStringFS();
                best_file += ".csv";
                ga::program::write_history(best_file, best_solution);
                
                std::string worst_file{"./ga25_worsts_history_"};
                worst_file += blt::system::getTimeStringFS();
                worst_file += ".csv";
                ga::program::write_history(worst_file, worst_best_solution);
            } else
            {
                BLT_INFO("Not a command.");
                continue;
            }
        }
    }
    
}
