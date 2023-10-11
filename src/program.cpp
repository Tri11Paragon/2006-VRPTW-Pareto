/*
 * Created by Brett on 30/09/23.
 * Licensed under GNU General Public License V3.0
 * See LICENSE file for license detail
 */
#include <program.h>

#include <blt/std/logging.h>
#include <valarray>
#include <blt/std/random.h>
#include <algorithm>

namespace ga
{
    
    std::int32_t capacity;
    std::vector<record> records;
    population current_population;
    
    double calculate_distance(const route& r)
    {
        // distance between first customer and the depot
        double dist = distance(0, r.customers[0]);
        for (size_t i = 1; i < r.customers.size(); i++)
        {
            dist += distance(r.customers[i - 1], r.customers[i]);
        }
        // distance between last customer and the depot
        dist += distance(r.customers[r.customers.size() - 1], 0);
        return dist;
    }
    
    double validate_route(const route& r)
    {
        if (r.customers.empty())
            return 0;
        const double dueTime = records[0].due;
        double used_capacity = 0;
        double lastDepartTime = 0;
        for (const auto& v : r.customers)
        {
            const auto& record = records[v];
            if (used_capacity + record.demand > capacity || lastDepartTime + record.service_time > record.due ||
                lastDepartTime + record.service_time > dueTime)
                return std::numeric_limits<double>::max(); // by returning max we will never use this solution. it also remains possible to check for error
            used_capacity += record.demand;
            lastDepartTime = std::max(lastDepartTime, record.ready) + record.service_time;
        }
        return calculate_distance(r);
    }
    
    double distance(std::int32_t c1, std::int32_t c2)
    {
        const auto& customer1 = records[c1];
        const auto& customer2 = records[c2];
        auto x = customer1.x - customer2.x;
        auto y = customer1.y - customer2.y;
        return std::sqrt(x * x + y * y);
    }
    
    std::vector<route> constructRoute(const chromosome& c)
    {
        std::vector<route> routes;
        
        const double dueTime = records[0].due;
        
        // phase 1
        int index = 0;
        while (index < CUSTOMER_COUNT)
        {
            route currentRoute;
            
            double currentCapacity = 0;
            double lastDepartTime = records[0].ready;
            
            while (index < CUSTOMER_COUNT)
            {
                const auto& r = records[c.genes[index]];
                // constraint violated, add route and reset
                // we assume when a vehicle leaves it will teleport to the next destination immediately but must be able to service BEFORE closing
                // if this isn't the intended behaviour remove the r.service_time from the second condition
                // lastDepartTime + r.service_time is consistent with "and must return before or at time bn+1"
                if (currentCapacity + r.demand > capacity || lastDepartTime + r.service_time > r.due || lastDepartTime + r.service_time > dueTime)
                    break;
                currentCapacity += r.demand;
                // wait until the server opens, add the service time, move on
                lastDepartTime = std::max(lastDepartTime, r.ready) + r.service_time;
                currentRoute.customers.push_back(c.genes[index++]);
            }
            currentRoute.total_distance = calculate_distance(currentRoute);
            routes.push_back(currentRoute);
        }
        
        // phase 2
        for (int i = 1; i < routes.size(); i++)
        {
            auto& route1 = routes[i - 1];
            auto& route2 = routes[i];
            
            auto val = route1.customers.back();
            auto val_itr = route2.customers.insert(route2.customers.begin(), val);
            route1.customers.pop_back();
            auto r1_td = validate_route(route1);
            auto r2_td = validate_route(route2);
            
            // network change is not accepted
            if (r1_td + r2_td > route1.total_distance + route2.total_distance)
            {
                route1.customers.push_back(val);
                route2.customers.erase(val_itr);
            } else
            {
                route1.total_distance = r1_td;
                route2.total_distance = r2_td;
                BLT_TRACE("route accepted!");
            }
        }
        
        return routes;
    }
    
    chromosome createRandomChromosome()
    {
        static std::random_device dev;
        static std::mt19937_64 engine(dev());
        
        std::vector<std::int32_t> unused;
        for (int i = 1; i <= CUSTOMER_COUNT; i++)
            unused.push_back(i);
        
        chromosome ca{};
        
        // pick genes in random order
        for (int& gene : ca.genes)
        {
            std::uniform_int_distribution dist(0, (int) unused.size() - 1);
            auto index = dist(engine);
            gene = unused[index];
            std::iter_swap(unused.begin() + index, unused.end() - 1);
            unused.pop_back();
        }
        
        return ca;
    }
    
    void init(std::int32_t c, std::vector<record>&& r)
    {
        capacity = c;
        records = std::move(r);
        
        current_population.pops.reserve(POPULATION_SIZE);
        
        for (int i = 0; i < POPULATION_SIZE; i++)
            current_population.pops.emplace_back(createRandomChromosome(), std::vector<route>{});
        
    }
    
    void destroy()
    {
    
    }
    
    int execute()
    {
        for (int _ = 0; _ < GENERATION_COUNT; _++)
        {
            // step 1. Transform each chromosome into feasible network configuration
            // by applying the routing scheme;
            for (auto& c : current_population.pops)
            {
                c.routes = constructRoute(c.c);
                c.rank = 0;
            }
            
            // Evaluate fitness of the individuals of POP;
            rankPopulation();
            
            return 0;
        }
        
        return 0;
    }
    
    void rankPopulation()
    {
        population pop = current_population;
        
        pop.pops.pop_back();
        
        BLT_INFO("%d, %d", pop.pops.size(), current_population.pops.size());
        
        int currentRank = 1;
        int N = POPULATION_SIZE;
        int m = N;
        while (N != 0)
        {
            for (int i = 0; i < m; i++)
            {
                
                N--;
            }
            
            auto pos = pop.pops.begin();
            while(true)
            {
                // making this O(n) instead of O(n^2)
                auto p = std::find_if(pos, pop.pops.end(), [&currentRank](const individual& v) {
                    return v.rank == currentRank;
                });
                if (p == pop.pops.end())
                    break;
                // swaps the value not the iterator
                std::iter_swap(p, pop.pops.end()-1);
                pop.pops.pop_back();
                // we can now continue the search from the found pos
                pos = p;
            }
            currentRank++;
            m = N;
        }
    }
    
}
