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

static std::random_device random_dev;
static std::mt19937_64 dev(random_dev());
static std::uniform_int_distribution<int32_t> dist(0, std::numeric_limits<int32_t>::max());

constexpr std::uint32_t MAX_SORT = 32;
constexpr std::uint32_t MAX_INVOCATIONS = 1000000;

static std::vector<int32_t> big_vec;

#define SORT_VALUES(NAME) auto NAME = std::accumulate(big_vec.begin(), big_vec.end(), 0);

static uint64_t global_total = 0;

class base
{
    public:
        virtual void func()
        {
            SORT_VALUES(BIG);
            global_total += BIG;
            //BLT_TRACE("Base added to %d", BIG);
        }
};

class derived : public base
{
    public:
        void func() override
        {
            SORT_VALUES(SMALL);
            global_total += SMALL;
            //BLT_TRACE("Derived added to %d", SMALL);
        }
};

void callWithAny(const std::any& func)
{
    any_cast<base*>(func)->func();
}

int main(int argc, const char** argv)
{
    for (int i = 0; i < MAX_SORT; i++)
        big_vec.push_back(dist(dev));
    blt::arg_parse parser;
    
    parser.addArgument(blt::arg_builder("--customers", "-c").setAction(blt::arg_action_t::STORE).setNArgs(1)
                                                            .setHelp("Set the number of customers. (Default: 100)")
                                                            .setDefault("100").build());
    
    auto args = parser.parse_args(argc, argv);
    
    ga::program p(args.get<int32_t>("customers"));
    
    base b;
    derived d;
    
    base* bp = &d;
    
    blt::profile_t profile("Type Erasure");
    
    auto iv_base = blt::createInterval(profile, "Calling Base Class");
    blt::startInterval(iv_base);
    BLT_START_INTERVAL("Type Erasure", "Calling Base Class");
    for (int i = 0; i < MAX_INVOCATIONS; i++)
    {
        b.func();
    }
    BLT_END_INTERVAL("Type Erasure", "Calling Base Class");
    blt::endInterval(iv_base);
    
    
    
    auto iv_derived = blt::createInterval(profile, "Calling Derived Class");
    blt::startInterval(iv_derived);
    BLT_START_INTERVAL("Type Erasure", "Calling Derived Class");
    for (int i = 0; i < MAX_INVOCATIONS; i++)
    {
        d.func();
    }
    BLT_END_INTERVAL("Type Erasure", "Calling Derived Class");
    blt::endInterval(iv_derived);
    
    
    
    auto iv_derived_through_base = blt::createInterval(profile, "Calling Derived Through Base");
    blt::startInterval(iv_derived_through_base);
    for (int i = 0; i < MAX_INVOCATIONS; i++)
    {
        bp->func();
    }
    blt::endInterval(iv_derived_through_base);
    
    
    
    auto iv_base_any = blt::createInterval(profile, "Calling Base ANY");
    blt::startInterval(iv_base_any);
    for (int i = 0; i < MAX_INVOCATIONS; i++)
    {
        callWithAny(&b);
    }
    blt::endInterval(iv_base_any);
    
    
    
    auto iv_derived_any = blt::createInterval(profile, "Calling Derived ANY");
    blt::startInterval(iv_derived_any);
    for (int i = 0; i < MAX_INVOCATIONS; i++)
    {
        callWithAny((base*) &d);
    }
    blt::endInterval(iv_derived_any);
    
    std::cout << global_total << "\n";
    blt::printProfile(profile);
    BLT_PRINT_PROFILE("Type Erasure");
    
    return p.execute();
}
