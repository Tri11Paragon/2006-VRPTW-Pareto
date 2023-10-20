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
#include "blt/std/logging.h"
#include "blt/std/format.h"
#include <queue>
#include <vector>
#include <thread>
#include <mutex>

struct datagram
{
    int32_t capacity;
    std::vector<std::string> problems;
};

namespace blt
{
    inline std::string filename(const std::string& path){
        if (loggingFormat.printFullFileName)
            return path;
        auto paths = split(path, "/");
        auto final = paths[paths.size()-1];
        if (final == "/")
            return paths[paths.size()-2];
        return final;
    }
}

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
                std::queue<datagram> data;
                data.emplace(
                    200,
                    std::vector<std::string>{
                        "../problems/r101.set",
                        "../problems/r102.set",
                        "../problems/r103.set",
                        "../problems/r104.set",
                        "../problems/r105.set",
                        "../problems/r106.set",
                        "../problems/r107.set",
                        "../problems/r108.set",
                        "../problems/r109.set",
                        "../problems/r110.set",
                        "../problems/r111.set",
                        "../problems/c101.set",
                        "../problems/c102.set",
                        "../problems/c103.set",
                        "../problems/c104.set",
                        "../problems/c105.set",
                        "../problems/c106.set",
                        "../problems/c107.set",
                        "../problems/c108.set",
                        "../problems/rc101.set",
                        "../problems/rc102.set",
                        "../problems/rc103.set",
                        "../problems/rc104.set",
                        "../problems/rc105.set",
                        "../problems/rc106.set",
                        "../problems/rc107.set",
                        "../problems/rc108.set",
                    }
                );
                data.emplace(
                    1000,
                    std::vector<std::string>{
                            "../problems/r201.set",
                            "../problems/r202.set",
                            "../problems/r203.set",
                            "../problems/r204.set",
                            "../problems/r205.set",
                            "../problems/r206.set",
                            "../problems/r207.set",
                            "../problems/r208.set",
                            "../problems/r209.set",
                            "../problems/r210.set",
                            "../problems/r211.set",
                            "../problems/rc201.set",
                            "../problems/rc202.set",
                            "../problems/rc203.set",
                            "../problems/rc204.set",
                            "../problems/rc205.set",
                            "../problems/rc206.set",
                            "../problems/rc207.set",
                            "../problems/rc208.set"
                    }
                );
                data.emplace(
                    700,
                    std::vector<std::string>{
                            "../problems/c101.set",
                            "../problems/c102.set",
                            "../problems/c103.set",
                            "../problems/c104.set",
                            "../problems/c105.set",
                            "../problems/c106.set",
                            "../problems/c107.set",
                            "../problems/c108.set"
                    }
                );
                std::mutex data_lock;
            } else
            {
                BLT_INFO("Not a command.");
                continue;
            }
        }
    }
    
}
