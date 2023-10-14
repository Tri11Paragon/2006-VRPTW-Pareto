/*
 * Created by Brett on 30/09/23.
 * Licensed under GNU General Public License V3.0
 * See LICENSE file for license detail
 */
#include <program.h>

#include <blt/std/logging.h>
#include <valarray>
#include <blt/std/random.h>
#include <blt/std/memory.h>
#include <algorithm>

namespace ga
{
    
    std::int32_t capacity;
    std::vector<record> records;
    population current_population;
    
    template<typename T>
    void print(const T& t)
    {
        for (const auto& v : t)
        {
            BLT_INFO_STREAM << v << " ";
        }
        BLT_INFO_STREAM << "\n";
    }
    
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
    
    /**
     * u dominates v iff ∀i ∈ (1, ..., k) : ui ≤ vi ∧ ∃i ∈ (1, ..., k) : ui < vi
     * @return if u is dominated by v
     */
    bool dominates(const individual& u, const individual& v)
    {
        auto u_distance = u.total_routes_distance;
        auto v_distance = v.total_routes_distance;
        auto u_vehicles = u.routes.size();
        auto v_vehicles = v.routes.size();
        // ∀i ∈ (1, ..., k) : ui ≤ v   ^   ∃i ∈ (1, ..., k) : ui < vi
        return (u_distance <= v_distance && u_vehicles <= v_vehicles) && (u_distance < v_distance || u_vehicles < v_vehicles);
    }
    
    bool is_non_dominated(size_t v)
    {
        for (int i = 0; i < current_population.pops.size(); i++)
        {
            // if i dominates by some pop i, then v cannot be non-dominated
            if (v != i && dominates(current_population.pops[i], current_population.pops[v]))
                return false;
        }
        return true;
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
                if ((currentCapacity + r.demand > capacity || lastDepartTime + r.service_time > r.due || lastDepartTime + r.service_time > dueTime))
                {
//                    BLT_INFO("%f + %f > %f", lastDepartTime, r.service_time, r.due);
//                    BLT_INFO_STREAM << currentRoute.customers.size() << " " << (currentCapacity + r.demand > capacity)
//                                    << (lastDepartTime + r.service_time > r.due) << (lastDepartTime + r.service_time > dueTime) << "\n";
                    break;
                }
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
//                BLT_TRACE("route accepted!");
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
            current_population.pops.emplace_back(createRandomChromosome());
        
    }
    
    void reconstruct_populations()
    {
        for (auto& c : current_population.pops)
        {
            c.rank = 0;
            c.total_routes_distance = 0;
            c.routes = constructRoute(c.c);
            for (const auto& r : c.routes)
                c.total_routes_distance += r.total_distance;
            
            //BLT_TRACE("Current population rank %d, total dist %f, with %d routes", c.rank, c.total_routes_distance, c.routes.size());
        }
    }
    
    void destroy()
    {
    
    }
    
    int execute()
    {
        for (int _ = 0; _ < GENERATION_COUNT; _++)
        {
            BLT_TRACE("Constructing network");
            // step 1. Transform each chromosome into feasible network configuration
            // by applying the routing scheme;
            reconstruct_populations();
            
            BLT_TRACE("Evaluating fitness");
            // Evaluate fitness of the individuals of POP;
            rankPopulation();
            BLT_DEBUG("Currently, In generation (%d), the best individuals are:", _ + 1);
            for (int i = 0; i < POPULATION_SIZE && current_population.pops[i].rank == 1; i++)
                    BLT_DEBUG("\t(%d): Total Distance %f | Total Route %d", i + 1, current_population.pops[i].total_routes_distance,
                              current_population.pops[i].routes.size());
            
            BLT_TRACE("Create population");
            population new_pop;
            keepElites(new_pop, ELITE_COUNT); // GREETINGS
            
            BLT_TRACE("Applying Crossovers");
            while (new_pop.pops.size() < POPULATION_SIZE)
                applyCrossover(new_pop);
            BLT_TRACE("Applying mutations");
            applyMutation(new_pop);
            
            while (new_pop.pops.size() > current_population.pops.size())
                new_pop.pops.pop_back();
            
            //BLT_DEBUG("New pop %d old pop %d", new_pop.pops.size(), current_population.pops.size());
            
            current_population = new_pop;
            
            //return 0;
        }
        reconstruct_populations();
        rankPopulation();
        
        BLT_INFO("The best individuals we found are:");
        for (int i = 0; i < POPULATION_SIZE && current_population.pops[i].rank == 1; i++)
        {
            std::string route_values;
            for (const auto& r : current_population.pops[i].routes)
            {
                auto d = validate_route(r);
                if (d == 0 || d == std::numeric_limits<double>::max())
                {
                    BLT_ERROR("Failure in pop (%d), route value is invalid!", i + 1);
                    //return -1;
                } else
                {
                    route_values += std::to_string(d) += " ";
                }
            }
            BLT_INFO("\t(%d): Total Distance %f | Total Route %d", i + 1, current_population.pops[i].total_routes_distance,
                     current_population.pops[i].routes.size());
            //BLT_INFO("\t\t%s", route_values.c_str());
        }
        
        return 0;
    }
    
    void rankPopulation()
    {
        population& pop = current_population;
        population ranked_pops;
        
        int currentRank = 1;
        int N = POPULATION_SIZE;
        int m = N;
        while (N != 0)
        {
            //BLT_DEBUG("Running on %d with total pops left %d new pops %d", currentRank, pop.pops.size(), ranked_pops.pops.size());
            for (int i = 0; i < m; i++)
            {
                if (is_non_dominated(i))
                {
                    current_population.pops[i].rank = currentRank;
                    ranked_pops.pops.push_back(current_population.pops[i]);
                    N--;
                }
            }
            // remove all with this current rank
            std::erase_if(pop.pops, [&currentRank](const ga::individual& v) -> bool { return v.rank == currentRank; });
            currentRank++;
            m = N;
        }
        current_population = std::move(ranked_pops);
    }
    
    void keepElites(population& pop, size_t n)
    {
        // we are only going to keep one but we have the option for more. At this point the population values are ordered so we can take the first
        for (int i = 0; i < n; i++)
            pop.pops.push_back(current_population.pops[i]);
    }
    
    size_t select_pop(size_t tournament_size)
    {
        static std::random_device dev;
        static std::mt19937_64 engine(dev());
        static std::uniform_real_distribution float_dist(0.0, 1.0);
        static std::uniform_int_distribution int_dist(0, POPULATION_SIZE - 1);
        
        //  A set of K individuals are randomly selected from the population
        std::vector<int> buffer;
        buffer.reserve(tournament_size);
        while (buffer.size() < tournament_size)
        {
            auto selection = int_dist(engine);
            if (std::find(buffer.begin(), buffer.end(), selection) == buffer.end())
                buffer.push_back(selection);
        }
        // If r is less than 0.8 (0.8 set empirically), the fittest individual in the tournament set is then chosen as the one to be used for reproduction.
        if (float_dist(engine) < 0.8)
        {
            std::int32_t min_rank = std::numeric_limits<std::int32_t>::max();
            std::int32_t index = 0;
            for (int i = 0; i < buffer.size(); i++)
            {
                auto r = current_population.pops[buffer[i]].rank;
                if (r < min_rank)
                {
                    min_rank = r;
                    index = i;
                }
            }
            return index;
        } else
        {
            // Otherwise, any chromosome is chosen for reproduction from the tournament set.
            std::uniform_int_distribution t_int_dist(0, (int) tournament_size - 1);
            return buffer[t_int_dist(engine)];
        }
    }
    
    inline void remove_from(const route& r, individual& c)
    {
        for (std::int32_t to_remove : r.customers)
        {
            for (auto& cr : c.routes)
            {
                std::erase_if(cr.customers, [&to_remove](const std::int32_t v) -> bool { return v == to_remove; });
            }
        };
    }
    
    void insert_to(const route& r_in, individual& c_in)
    {
        for (std::int32_t v : r_in.customers)
        {
            // cache the route distance
            struct route_cache
            {
                double distance = 0;
                size_t route_index = 0;
                size_t insertion_index = 0;
            };
            
            std::vector<route_cache> possibleRoutes;
            for (int j = 0; j < c_in.routes.size(); j++)
            {
                const route& r = c_in.routes[j];
                for (int i = 0; i < r.customers.size(); i++)
                {
                    route r_copy = r;
                    r_copy.customers.insert(r_copy.customers.begin() + i, v);
                    double d = validate_route(r_copy);
                    if (d != std::numeric_limits<double>::max())
                        possibleRoutes.emplace_back(d, j, i);
                }
            }
            // no feasible route found, we must make a new one
            if (possibleRoutes.empty())
            {
                route new_route;
                new_route.customers.push_back(v);
                c_in.routes.push_back(new_route);
            } else
            {
                route_cache min;
                min.distance = std::numeric_limits<double>::max();
                for (const auto& r : possibleRoutes)
                {
                    if (r.distance < min.distance)
                        min = r;
                }
                c_in.routes[min.route_index].customers
                                            .insert(c_in.routes[min.route_index].customers.begin() + static_cast<long>(min.insertion_index), v);
            }
        }
    }
    
    void reconstruct_chromosome(individual& i)
    {
        std::memset(i.c.genes.data(), 0, sizeof(std::int32_t) * CUSTOMER_COUNT);
        for (const auto v : i.c.genes)
        {
            if (v != 0)
                BLT_ERROR("failure of setting");
        }
        int insertion_index = 0;
        for (const auto& route : i.routes)
        {
            for (const auto& customer : route.customers)
                i.c.genes[insertion_index++] = customer;
        }
    }
    
    void applyCrossover(population& pop)
    {
        static std::random_device dev;
        static std::mt19937_64 engine(dev());
        static std::uniform_real_distribution crossover_chance(0.0, 1.0);
        auto p1 = select_pop(TOURNAMENT_SIZE);
        auto p2 = select_pop(TOURNAMENT_SIZE);
        // make sure we don't create children with ourselves
        while (p2 == p1)
            p2 = select_pop(TOURNAMENT_SIZE);
        
        const auto& parent1 = current_population.pops[p1];
        const auto& parent2 = current_population.pops[p2];
        
        // don't apply crossover, just move the parents in unchanged.
        if (crossover_chance(engine) > CROSSOVER_RATE)
        {
            pop.pops.push_back(parent1);
            pop.pops.push_back(parent2);
            return;
        }
        
        std::uniform_int_distribution p1_r_select(0ul, parent1.routes.size() - 1);
        std::uniform_int_distribution p2_r_select(0ul, parent2.routes.size() - 1);
        
        const auto& r1 = parent1.routes[p1_r_select(engine)];
        const auto& r2 = parent2.routes[p2_r_select(engine)];
        
        individual c1 = parent1, c2 = parent2;
        
        // remove r1 from p2 and r2 from p1
        remove_from(r1, c2);
        remove_from(r2, c1);
        
        // now insert r1 back into p2 and r2 into p1 to create c1 and c2
        insert_to(r1, c2);
        insert_to(r2, c1);
        
        // finally reconstruct the chromosome using the routes will be done after mutation since mutation also modifies the routes.
        pop.pops.push_back(c1);
        pop.pops.push_back(c2);
    }
    
    void applyMutation(population& pop)
    {
        static std::random_device dev;
        static std::mt19937_64 engine(dev());
        static std::uniform_real_distribution mutation_chance(0.0, 1.0);
        for (auto& indv : pop.pops)
        {
            if (mutation_chance(engine) < MUTATION_RATE)
            {
                // run until we mutate a valid route.
                while (true)
                {
                    std::uniform_int_distribution select(0ul, indv.routes.size() - 1);
                    auto& route = indv.routes[select(engine)];
                    if (route.customers.size() <= 1)
                        continue;
                    auto route_copy = route;
                    if (route.customers.size() == 2)
                    {
                        // simple swap op.
                        auto t = route.customers[0];
                        route.customers[0] = route.customers[1];
                        route.customers[1] = t;
                    } else
                    {
                        // 2 - 3 customers
                        static std::uniform_int_distribution len(1, 2);
                        auto length = len(engine);
                        std::uniform_int_distribution point(0ul, route.customers.size() - 1 - length);
                        // inversion_start_point
                        auto isp = static_cast<long>(point(engine));
                        std::reverse(route.customers.begin() + isp, route.customers.begin() + isp + length + 1);
                    }
                    
                    // if it's not valid, reset.
                    if (validate_route(route) == std::numeric_limits<double>::max())
                    {
                        route = route_copy;
                    }
                    
                    break;
                }
            }
        }
        // rebuild all the chromosomes. this might actually cause problems?
        for (auto& i : pop.pops)
            reconstruct_chromosome(i);
    }
    
}
