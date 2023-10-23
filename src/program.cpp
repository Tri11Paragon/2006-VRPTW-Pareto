/*
 * Created by Brett on 30/09/23.
 * Licensed under GNU General Public License V3.0
 * See LICENSE file for license detail
 */
#include <program.h>

#include <blt/std/logging.h>
#include <valarray>
#include <blt/std/random.h>
#include <blt/std/string.h>
#include <blt/std/time.h>
#include <algorithm>
#include <fstream>
#include <ostream>
#include "blt/std/assert.h"
#include <unordered_set>

namespace ga
{
//#define HARD_VRPTW(lastDepartTime, route) (lastDepartTime + route.service_time)
#define HARD_VRPTW(lastDepartTime, route) (lastDepartTime)
    
    
    double program::distance(customerID_t c1, customerID_t c2)
    {
        const auto& customer1 = records[c1];
        const auto& customer2 = records[c2];
        auto x = customer1.x - customer2.x;
        auto y = customer1.y - customer2.y;
        return std::sqrt(x * x + y * y);
    }
    
    double program::calculate_distance(const route& r)
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
    
    bool program::validate_route(const route& r)
    {
        // by returning max we will never use this solution. it also remains possible to check for error
        if (r.customers.empty())
            return false;
        const double dueTime = records[0].due;
        double used_capacity = 0;
        double arrivalTime = 0;
        for (const auto& v : r.customers)
        {
            const auto& record = records[v];
            // capacity constraints
            if (used_capacity + record.demand > capacity)
                return false;
            // arrival constraints
            if (arrivalTime > record.due)
                return false;
            // return time constraints
            if (arrivalTime + record.service_time > dueTime)
                return false;
            used_capacity += record.demand;
            // handle early arrival time by making it wait.
            arrivalTime = std::max(arrivalTime, record.ready) + record.service_time;
//            BLT_TRACE("%f %f", record.ready, record.due);
//            BLT_DEBUG("Currently on %d with arrivalTime %f with used capacity %f", v, arrivalTime, used_capacity);
        }
        return true;
    }
    
    /**
     * u dominates v iff ∀i ∈ (1, ..., k) : ui ≤ vi ∧ ∃i ∈ (1, ..., k) : ui < vi
     * @return if u is dominated by v
     */
    bool program::dominates(const individual& u, const individual& v)
    {
        auto u_distance = u.total_routes_distance;
        auto v_distance = v.total_routes_distance;
        auto u_vehicles = u.routes.size();
        auto v_vehicles = v.routes.size();
        // ∀i ∈ (1, ..., k) : ui ≤ v   ^   ∃i ∈ (1, ..., k) : ui < vi
        return (u_distance <= v_distance && u_vehicles <= v_vehicles) && (u_distance < v_distance || u_vehicles < v_vehicles);
    }
    
    bool program::is_non_dominated(customerID_t v)
    {
        for (customerID_t i = 0; i < static_cast<customerID_t>(current_population.pops.size()); i++)
        {
            // if v is dominated by some pop i, then v cannot be non-dominated
            if (v != i && dominates(current_population.pops[i], current_population.pops[v]))
                return false;
        }
        return true;
    }
    
    fitness_t program::weighted_sum_fitness(individual& v)
    {
        return ALPHA * static_cast<fitness_t>(v.routes.size()) + BETA * v.total_routes_distance;
    }
    
    customerID_t program::select_pop(size_t tournament_size)
    {
        
        //  A set of K individuals are randomly selected from the population
        std::vector<customerID_t> buffer;
        buffer.reserve(tournament_size);
        while (buffer.size() < tournament_size)
        {
            auto selection = engine.getInt(0, POPULATION_SIZE - 1);
            if (std::find(buffer.begin(), buffer.end(), selection) == buffer.end())
                buffer.push_back(selection);
        }
        // If r is less than 0.8 (0.8 set empirically), the fittest individual in the tournament set is then chosen as the one to be used for reproduction.
        if (engine.getDouble(0, 1) < 0.8)
        {
            size_t index = 0;
            if (using_fitness)
            {
                fitness_t min_fitness = std::numeric_limits<fitness_t>::max();
                for (size_t i = 0; i < buffer.size(); i++)
                {
                    auto f = current_population.pops[buffer[i]].fitness;
                    if (f < min_fitness)
                    {
                        min_fitness = f;
                        index = i;
                    }
                }
            } else
            {
                rank_t min_rank = std::numeric_limits<rank_t>::max();
                for (size_t i = 0; i < buffer.size(); i++)
                {
                    auto r = current_population.pops[buffer[i]].rank;
                    if (r < min_rank)
                    {
                        min_rank = r;
                        index = i;
                    }
                }
            }
            return buffer[index];
        } else
        {
            // Otherwise, any chromosome is chosen for reproduction from the tournament set.
            return buffer[engine.getInt(0, (int) tournament_size - 1)];
        }
    }
    
    void program::remove_from(const route& r, individual& c)
    {
        for (std::int32_t to_remove : r.customers)
        {
            for (auto& cr : c.routes)
            {
                std::erase_if(cr.customers, [&to_remove](const std::int32_t v) -> bool { return v == to_remove; });
            }
        };
    }
    
    void program::insert_to(const route& r_in, individual& c_in)
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
            for (size_t j = 0; j < c_in.routes.size(); j++)
            {
                const route& r = c_in.routes[j];
                for (size_t i = 0; i < r.customers.size(); i++)
                {
                    route r_copy = r;
                    r_copy.customers.insert(r_copy.customers.begin() + static_cast<long>(i), v);
                    if (validate_route(r_copy))
                        possibleRoutes.emplace_back(calculate_distance(r_copy), j, i);
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
    
    void program::reconstruct_populations()
    {
        for (auto& c : current_population.pops)
        {
            c.rank = 0;
            c.fitness = 0;
            c.total_routes_distance = 0;
            c.routes = constructRoute(c.c);
            for (const auto& r : c.routes)
                c.total_routes_distance += r.total_distance;
        }
    }
    
    void program::rebuild_population_chromosomes(population& pop)
    {
        // rebuild all the chromosomes. this might actually cause problems?
        for (auto& i : pop.pops)
            reconstruct_chromosome(i);
    }
    
    void program::add_step_to_history()
    {
        generation_point gen_point{count};
        double best_distAvg = 0;
        double avg_distAvg = 0;
        size_t best_routes = 0;
        size_t avg_routes = 0;
        size_t cnt = 0;
        size_t best_cnt = 0;
        fitness_t fit = std::numeric_limits<double>::max();
        for (const auto& i : current_population.pops)
            fit = std::min(fit, i.fitness);
        for (int i = 0; i < POPULATION_SIZE; i++)
        {
            auto& currentP = current_population.pops[i];
            gen_point.indv.push_back(individual_point{currentP.total_routes_distance, currentP.fitness, currentP.rank, currentP.routes.size()});
            auto total_dist = currentP.total_routes_distance;
            auto total_routes = currentP.routes.size();
            avg_distAvg += total_dist;
            avg_routes += total_routes;
            cnt++;
            auto lf = currentP.fitness;
            if (using_fitness && !(lf >= fit - EPSILON && lf <= fit + EPSILON))
                continue;
            if (!using_fitness && currentP.rank != 1)
                continue;
            best_distAvg += total_dist;
            best_routes += total_routes;
            best_cnt++;
        }
        best_history.push_back({best_distAvg / static_cast<double>(best_cnt), best_routes / best_cnt, count});
        avg_history.push_back({avg_distAvg / static_cast<double>(cnt), avg_routes / cnt, count});
    }
    
    void program::reconstruct_chromosome(individual& i)
    {
        std::memset(i.c.genes.data(), 0, sizeof(customerID_t) * CUSTOMER_COUNT);
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
    
    std::vector<route> program::constructRoute(const chromosome& c)
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
                // capacity constraints
                if (currentCapacity + r.demand > capacity)
                    break;
                // arrival constraint
                if (lastDepartTime > r.due)
                    break;
                // return constraint
                if (lastDepartTime + r.service_time > dueTime)
                    break;
                currentCapacity += r.demand;
                // wait until the customer opens, add the service time, move on
                lastDepartTime = std::max(lastDepartTime, r.ready) + r.service_time;
                currentRoute.customers.push_back(c.genes[index++]);
            }
            if (!validate_route(currentRoute))
                BLT_WARN("Route is invalid!");
            currentRoute.total_distance = calculate_distance(currentRoute);
            routes.push_back(currentRoute);
        }
        
        // phase 2
        for (size_t i = 1; i < routes.size(); i++)
        {
            auto& route1 = routes[i - 1];
            auto rc1 = routes[i - 1];
            auto& route2 = routes[i];
            auto rc2 = routes[i];
            
//            auto back = rc1.customers.back();
//            auto front = rc2.customers.front();
            
//            rc1.customers.pop_back();
//            std::erase(rc2.customers, front);
//
//            rc1.customers.push_back(front);
//            rc2.customers.push_back(back);
            std::swap(rc1.customers.back(), rc1.customers.front());
            
            // if they are not valid, skip
            if (!validate_route(rc1) || !validate_route(rc2))
                continue;
            
            rc1.total_distance = calculate_distance(rc1);
            rc2.total_distance = calculate_distance(rc2);
            
            // reject changes if not better
            if (rc1.total_distance + rc2.total_distance >= route1.total_distance + route2.total_distance)
                continue;
            
            // accept changes
            route1 = rc1;
            route2 = rc2;

            BLT_ASSERT(validate_route(route1) && validate_route(route2));
        }
        
        return routes;
    }
    
    chromosome program::createRandomChromosome()
    {
        chromosome ca{};
        
        std::vector<std::int32_t> unused;
        for (int i = 1; i <= CUSTOMER_COUNT; i++)
            unused.push_back(i);
        
        if (engine.getDouble(0, 1) < 0.9)
        {
            // pick genes in random order
            for (int& gene : ca.genes)
            {
                auto index = engine.getInt(0, (int) unused.size() - 1);
                gene = unused[index];
                std::iter_swap(unused.begin() + index, unused.end() - 1);
                unused.pop_back();
            }
        } else
        {
            size_t insert_index = 0;
            while (!unused.empty())
            {
                // Randomly remove a customer ci ∈ C
                auto index = engine.getInt(0, (int) unused.size() - 1);
                auto ci = unused[index];
                std::iter_swap(unused.begin() + index, unused.end() - 1);
                unused.pop_back();
                
                // Add customer node ci to the chromosome string l;
                ca.genes[insert_index++] = ci;
                
                std::vector<std::int32_t> close;
                
                // Within an empirically decided Euclidean radius centered around ci, choose the nearest customer cj , where cj 6 ∈ l
                static constexpr double MAX_DISTANCE = 25;
                for (const auto& v : unused)
                {
                    if (distance(v, ci) <= MAX_DISTANCE)
                    {
                        close.push_back(v);
                    }
                }
                
                std::sort(close.begin(), close.end(), [&ci, this](const auto v1, const auto v2) -> bool {
                    return distance(v1, ci) < distance(v2, ci);
                });
                
                // If cj D.N.E then goto 3
                if (close.empty())
                    continue;
                
                ca.genes[insert_index++] = close[0];
                std::erase(unused, close[0]);
            }
        }
        
        return ca;
    }
    
    void program::executeStep()
    {
        // step 1. Transform each chromosome into feasible network configuration
        // by applying the routing scheme;
        reconstruct_populations();
        
        calculatePopulationFitness();
        // Evaluate fitness of the individuals of POP;
        rankPopulation();
        
        add_step_to_history();
        
        population new_pop;
        keepElites(new_pop, ELITE_COUNT); // GREETINGS
        
        while (static_cast<std::int32_t>(new_pop.pops.size()) < POPULATION_SIZE)
            applyCrossover(new_pop);
        
        applyMutation(new_pop);
        
        rebuild_population_chromosomes(new_pop);
        //reconstruct_populations();
        //rankPopulation();
        
        // secondary is applied to the chromosome itself not the best_routes
        //applySecondaryMutation(new_pop);
        
        
        //while (new_pop.pops.size() > current_population.pops.size())
        //    new_pop.pops.pop_back();
        
        current_population = new_pop;
        count++;
    }
    
    void program::rankPopulation()
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
    
    void program::keepElites(population& pop, size_t n)
    {
//        constexpr bool useRank = false;
//        if constexpr (useRank){
//            for (size_t j = 0; j < n; j++)
//                for (size_t i = 0; i < current_population.pops.size() && current_population.pops[i].rank == 1; i++)
//                    pop.pops.push_back(current_population.pops[i]);
//        } else
        // we are only going to keep one, but we have the option for more. At this point the population values are ordered so we can take the first
        for (size_t i = 0; i < n && current_population.pops[i].rank == 1; i++)
            pop.pops.push_back(current_population.pops[i]);
    }
    
    bool routes_same(const route& r1, const route& r2)
    {
        if (r1.customers.size() != r2.customers.size())
            return false;
        for (size_t i = 0; i < r1.customers.size(); i++)
            if (r1.customers[i] != r2.customers[i])
                return false;
        return true;
    }
    
    void program::applyCrossover(population& pop)
    {
        auto p1 = select_pop(TOURNAMENT_SIZE);
        auto p2 = select_pop(TOURNAMENT_SIZE);
        // make sure we don't create children with ourselves
        while (p2 == p1)
            p2 = select_pop(TOURNAMENT_SIZE);
        
        const auto& parent1 = current_population.pops[p1];
        const auto& parent2 = current_population.pops[p2];
        
        // don't apply crossover, just move the parents in unchanged.
        if (engine.getDouble(0, 1) > CROSSOVER_RATE)
        {
            if (pop.pops.size() % 2 == 0)
                pop.pops.push_back(parent1);
            pop.pops.push_back(parent2);
            return;
        }
        
        auto route1Index = engine.getLong(0ul, parent1.routes.size() - 1);
        auto route2Index = engine.getLong(0ul, parent2.routes.size() - 1);
        
        while (routes_same(parent1.routes[route1Index], parent2.routes[route2Index]))
            route2Index = engine.getLong(0ul, parent2.routes.size() - 1);
        
        const auto& r1 = parent1.routes[route1Index];
        const auto& r2 = parent2.routes[route2Index];
            
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
    
    void program::applyMutation(population& pop)
    {
        for (auto& indv : pop.pops)
        {
            if (engine.getDouble(0, 1) <= MUTATION_RATE)
            {
                // run until we mutate a valid route.
                while (true)
                {
                    auto& route = indv.routes[engine.getLong(0ul, indv.routes.size() - 1)];
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
                        auto length = engine.getInt(1, 2);
                        // inversion_start_point
                        auto isp = static_cast<long>(engine.getLong(0ul, route.customers.size() - 1 - length));
                        std::reverse(route.customers.begin() + isp, route.customers.begin() + isp + length + 1);
                    }
                    
                    // if it's not valid, reset.
                    if (validate_route(route))
                    {
                        route = route_copy;
                    }
                    
                    break;
                }
            }
        }
    }
    
    void program::applySecondaryMutation(population& pop)
    {
        for (auto& indv : pop.pops)
        {
            // do not destroy out best routes.
            if (indv.rank == 1)
                continue;
            // add some genetic diversity for when we converge to a local min
            if (engine.getDouble(0, 1) <= MUTATION2_RATE)
            {
                auto select = engine.getInt(0, 2);
                if (select == 0)
                {
                    indv.c = createRandomChromosome();
                } else
                {
                    auto p1 = engine.getLong(0, indv.c.genes.size() - 1);
                    auto p2 = p1;
                    while (p2 <= p1)
                        p2 = engine.getLong(0, indv.c.genes.size());
                    std::vector<std::int32_t> values;
                    for (size_t i = p1; i < p2; i++)
                        values.push_back(indv.c.genes[i]);
                    if (select == 1)
                    {
                        // randomly generate a new sub chromosome string
                        for (size_t i = p1; i < p2; i++)
                        {
                            auto index = engine.getLong(0, values.size() - 1);
                            indv.c.genes[i] = values[index];
                            std::iter_swap(values.begin() + static_cast<long>(index), values.end() - 1);
                            values.pop_back();
                        }
                    } else
                    {
                        // invert a section
                        for (size_t i = p1; i < p2; i++)
                        {
                            indv.c.genes[i] = values.back();
                            values.pop_back();
                        }
                    }
                    BLT_ASSERT(values.empty());
                }
            }
            std::unordered_set<std::int32_t> existingValues;
            for (const auto gene : indv.c.genes)
            {
                BLT_ASSERT(!existingValues.contains(gene));
                existingValues.insert(gene);
            }
        }
    }
    
    void program::print()
    {
        reconstruct_populations();
        calculatePopulationFitness();
        rankPopulation();
        double averageDist = 0;
        size_t avgVeh = 0;
        for (int i = 0; i < POPULATION_SIZE; i++)
        {
            BLT_DEBUG("\t(%d: %d | %f): Total Distance %f | Total Routes %d", i + 1, current_population.pops[i].rank,
                      current_population.pops[i].fitness,
                      current_population.pops[i].total_routes_distance,
                      current_population.pops[i].routes.size());
            averageDist += current_population.pops[i].total_routes_distance;
            avgVeh += current_population.pops[i].routes.size();
        }
        BLT_INFO("Total/Avg Dist: (%f/%f), Total/Avg Routes: (%d/%d)", averageDist,
                 averageDist / static_cast<double>(POPULATION_SIZE), avgVeh, avgVeh / POPULATION_SIZE);
        int printed = 0;
        if (using_fitness)
        {
            auto lowest = current_population.pops[0];
            for (const auto& i : current_population.pops)
            {
                if (i.fitness < lowest.fitness)
                    lowest = i;
            }
            BLT_INFO("Best in population (%f): Total distance %f | Total Routes %d", lowest.fitness, lowest.total_routes_distance,
                     lowest.routes.size());
        } else
        {
            for (int i = 0; i < POPULATION_SIZE && current_population.pops[i].rank == 1 && printed < 5; i++)
            {
                BLT_INFO("Best in population (%d): Total distance %f | Total Routes %d", printed, current_population.pops[i].total_routes_distance,
                         current_population.pops[i].routes.size());
                printed++;
            }
        }
    }
    
    void program::validate()
    {
        reconstruct_populations();
        rankPopulation();
        for (int i = 0; i < POPULATION_SIZE; i++)
        {
            std::string route_values;
            for (size_t j = 0; j < current_population.pops[i].routes.size(); j++)
            {
                const auto& r = current_population.pops[i].routes[j];
                if (validate_route(r))
                {
                    route_values += std::to_string(calculate_distance(r)) += " ";
                } else
                {
                    BLT_ERROR("Failure in pop (%d), route (%d) is invalid!", i + 1, j + 1);
                    constraintFailurePrint(r);
                }
            }
        }
        BLT_INFO("Validation complete.");
    }
    
    void program::write(const std::string& input)
    {
        reconstruct_populations();
        rankPopulation();
        const auto args = blt::string::split(input, ' ');
        std::string path = "./" + blt::system::getTimeStringFS();
        if (args.size() > 1)
            path = args[1];
        std::ofstream out(path);
        for (size_t i = 0; i < current_population.pops.size(); i++)
        {
            const auto& pop = current_population.pops[i];
            out << '(' << i << ") " << pop.rank << " " << pop.fitness << ": " << pop.total_routes_distance << " | " << pop.routes.size() << "\n";
            out << "\troutes:\n";
            for (const auto& r : pop.routes)
            {
                if (r.total_distance == 0)
                    BLT_WARN("We have a zero distance! %f", calculate_distance(r));
                out << "\t\t" << r.total_distance << "(valid? " << (validate_route(r) ? "true" : "false")
                    << "): ";
                for (const auto c : r.customers)
                    out << c << " ";
                out << "\n";
            }
            out << "\tchromosome string:\n\t\t";
            for (const auto v : pop.c.genes)
            {
                out << v << " ";
            }
            out << "\n";
        }
    }
    
    void program::write_history(const std::string& path, const std::vector<avg_point>& history)
    {
        std::ofstream out(path);
        out << "Generation,Distance,Routes\n";
        for (const auto& v : history)
            out << v.currentGen + 1 << ',' << v.distance << ',' << v.routes << '\n';
    }
    
    void program::writeHistory()
    {
        std::string best_file{"./ga_bests_history_"};
        best_file += blt::system::getTimeStringFS();
        best_file += ".csv";
        write_history(best_file, best_history);
        
        std::string avg_file{"./ga_avg_history_"};
        avg_file += blt::system::getTimeStringFS();
        avg_file += ".csv";
        write_history(avg_file, avg_history);
    }
    
    void program::calculatePopulationFitness()
    {
        for (auto& pop : current_population.pops)
            pop.fitness = weighted_sum_fitness(pop);
    }
    
    void program::reset()
    {
        current_population.pops.clear();
        for (int i = 0; i < POPULATION_SIZE; i++)
            current_population.pops.emplace_back(createRandomChromosome());
    }
    
    void program::constraintFailurePrint(const route& r)
    {
        // by returning max we will never use this solution. it also remains possible to check for error
        if (r.customers.empty())
            return;
        const double dueTime = records[0].due;
        double used_capacity = 0;
        double arrivalTime = 0;
        for (const auto& v : r.customers)
        {
            const auto& record = records[v];
            // capacity constraints
            if (used_capacity + record.demand > capacity)
            {
                BLT_INFO("\tCapacity Failure");
                return;
            }
            // arrival constraints
            if (arrivalTime > record.due)
            {
                BLT_INFO("\tArrival Failure");
                return;
            }
            // return time constraints
            if (arrivalTime + record.service_time > dueTime)
            {
                BLT_INFO("Return Failure");
                return;
            }
            used_capacity += record.demand;
            // handle early arrival time by making it wait.
            arrivalTime = std::max(arrivalTime, record.ready) + record.service_time;
//            BLT_TRACE("%f %f", record.ready, record.due);
//            BLT_DEBUG("Currently on %d with arrivalTime %f with used capacity %f", v, arrivalTime, used_capacity);
        }
    }
    
}
