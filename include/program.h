#pragma once
/*
 * Created by Brett on 30/09/23.
 * Licensed under GNU General Public License V3.0
 * See LICENSE file for license detail
 */

#ifndef INC_2006_VRPTW_PARETO_PROGRAM_H
#define INC_2006_VRPTW_PARETO_PROGRAM_H

#include <cstdint>
#include <loader.h>
#include <array>
#include <cstring>
#include <algorithm>
#include <blt/std/logging.h>
#include <blt/std/random.h>
#include <blt/std/string.h>
#include "blt/std/assert.h"

namespace ga
{
    
    typedef std::int32_t customerID_t;
    typedef std::int32_t rank_t;
    typedef double fitness_t;
    typedef double distance_t;
    
    static constexpr std::int32_t CUSTOMER_COUNT = 100;
    static constexpr std::int32_t DEFAULT_POPULATION_SIZE = 300;
    static constexpr std::int32_t DEFAULT_GENERATION_COUNT = 350;
    static constexpr std::int32_t DEFAULT_TOURNAMENT_SIZE = 4;
    static constexpr std::int32_t DEFAULT_ELITE_COUNT = 1;
    static constexpr double DEFAULT_CROSSOVER_RATE = 0.8;
    static constexpr double DEFAULT_MUTATION_RATE = 0.1;
    static constexpr double DEFAULT_MUTATION_2_RATE = 0.1;
    
    // weighted sum fitness
    static constexpr fitness_t ALPHA = 100;
    static constexpr fitness_t BETA = 0.001;
    constexpr double EPSILON = 0.0001;
    
    struct chromosome
    {
        std::array<customerID_t, CUSTOMER_COUNT> genes{};
    };
    
    struct route
    {
        std::vector<customerID_t> customers;
        distance_t total_distance = 0;
    };
    
    struct individual
    {
        chromosome c;
        std::vector<route> routes{};
        distance_t total_routes_distance = 0;
        rank_t rank = 0;
        fitness_t fitness = 0;
    };
    
    struct population
    {
        std::vector<individual> pops;
    };
    
    struct avg_point
    {
        double distance;
        size_t routes;
        size_t currentGen;
    };
    
    struct individual_point
    {
        double distance = 0;
        fitness_t fitness = 0;
        rank_t rank = 0;
        size_t routes = 0;
        
        static inline individual_point max()
        {
            return {std::numeric_limits<double>::max(), std::numeric_limits<fitness_t>::max(), std::numeric_limits<rank_t>::max(),
                    std::numeric_limits<size_t>::max()};
        }
        
        individual_point& operator+=(individual_point p)
        {
            distance += p.distance;
            routes += p.routes;
            return *this;
        }
        
        individual_point& operator/=(int32_t d)
        {
            distance /= static_cast<double>(d);
            routes /= static_cast<size_t>(d);
            return *this;
        }
    };
    
    struct generation_point
    {
        size_t currentGen;
        std::vector<individual_point> indv;
    };

#define RANDOM_STATIC static thread_local
//#define RANDOM_ENGINE() RANDOM_STATIC std::random_device dev; \
//    std::seed_seq seq{dev(), dev()}; \
//    RANDOM_STATIC std::mt19937_64 engine(seq);
    
    class random_engine
    {
        private:
        
        public:
            inline double getDouble(double min, double max)
            {
                RANDOM_STATIC std::random_device dev;
                RANDOM_STATIC std::seed_seq seq{dev(), dev()};
                RANDOM_STATIC std::mt19937_64 engine(seq);
                std::uniform_real_distribution dist(min, max);
                return dist(engine);
            }
            
            inline float getFloat(float min, float max)
            {
                return static_cast<float>(getDouble(min, max));
            }
            
            inline std::int32_t getInt(std::int32_t min, std::int32_t max)
            {
                RANDOM_STATIC std::random_device dev;
                RANDOM_STATIC std::seed_seq seq{dev(), dev()};
                RANDOM_STATIC std::mt19937_64 engine(seq);
                std::uniform_int_distribution dist(min, max);
                return dist(engine);
            }
            
            inline std::int64_t getLong(std::int64_t min, std::int64_t max)
            {
                RANDOM_STATIC std::random_device dev;
                RANDOM_STATIC std::seed_seq seq{dev(), dev()};
                RANDOM_STATIC std::mt19937_64 engine(seq);
                std::uniform_int_distribution dist(min, max);
                return dist(engine);
            }
            
            inline std::uint64_t getLong(std::uint64_t min, std::uint64_t max)
            {
                RANDOM_STATIC std::random_device dev;
                RANDOM_STATIC std::seed_seq seq{dev(), dev()};
                RANDOM_STATIC std::mt19937_64 engine(seq);
                std::uniform_int_distribution dist(min, max);
                return dist(engine);
            }
    };
    
    class program
    {
        private:
            double distance(customerID_t c1, customerID_t c2);
            
            double calculate_distance(const route& r);
            
            bool validate_route(const route& r);
            
            void constraintFailurePrint(const route& r);
            
            void validate_route(const std::string& str, std::vector<int32_t>& values)
            {
                BLT_TRACE("");
                auto strs = blt::string::split(str, ' ');
                auto lastLeaveTime = 0.0;
                auto cap = 0.0;
                for (const auto& i : strs)
                {
                    auto v = std::stoi(i);
                    auto r = records[v];
                    
                    values.push_back(v);
                    
                    BLT_TRACE_STREAM << "(" << i << ": " << r.ready << " | " << r.due << " but comes at " << lastLeaveTime << " and leaves "
                                     << (std::max(lastLeaveTime, r.ready) + r.service_time) << ") ";
                    
                    if (lastLeaveTime > r.due || lastLeaveTime > records[0].due)
                        BLT_ERROR("\nFailed Route");
                    
                    cap += r.demand;
                    lastLeaveTime = std::max(lastLeaveTime, r.ready) + r.service_time;
                }
                BLT_ASSERT(cap <= capacity);
                BLT_ASSERT(lastLeaveTime <= records[0].due);
                
                BLT_TRACE_STREAM << "\n";
                BLT_TRACE("%f %f", cap, lastLeaveTime);
            }
            
            static bool dominates(const individual& u, const individual& v);
            
            bool is_non_dominated(customerID_t v);
            
            static double weighted_sum_fitness(individual& v);
            
            customerID_t select_pop(size_t tournament_size);
            
            static void remove_from(const route& r, individual& c);
            
            void insert_to(const route& r_in, individual& c_in);
            
            void reconstruct_populations();
            
            static void reconstruct_chromosome(individual& i);
            
            static void rebuild_population_chromosomes(population& pop);
            
            void add_step_to_history();
        
        protected:
            std::vector<route> constructRoute(const chromosome& c);
            
            chromosome createRandomChromosome();
            
            void calculatePopulationFitness();
            
            void rankPopulation();
            
            void keepElites(population& pop, size_t n);
            
            void applyCrossover(population& pop);
            
            void applyMutation(population& pop);
            
            void applySecondaryMutation(population& pop);
        
        public:
            program(std::int32_t c, std::vector<record>&& r, bool usingFitness = false, std::int32_t popSize = DEFAULT_POPULATION_SIZE,
                    std::int32_t genCount = DEFAULT_GENERATION_COUNT, std::int32_t tourSize = DEFAULT_TOURNAMENT_SIZE,
                    std::int32_t eliteCount = DEFAULT_ELITE_COUNT, double crossoverRate = DEFAULT_CROSSOVER_RATE,
                    double mutationRate = DEFAULT_MUTATION_RATE, double mutation2Rate = DEFAULT_MUTATION_2_RATE):
                    POPULATION_SIZE(popSize), GENERATION_COUNT(genCount), TOURNAMENT_SIZE(tourSize), ELITE_COUNT(eliteCount),
                    CROSSOVER_RATE(crossoverRate), MUTATION_RATE(mutationRate), MUTATION2_RATE(mutationRate), using_fitness(usingFitness)
            {
                capacity = c;
                records = std::move(r);
                
                current_population.pops.reserve(POPULATION_SIZE);
                
                for (int i = 0; i < POPULATION_SIZE; i++)
                    current_population.pops.emplace_back(createRandomChromosome());
            }
            
            void executeStep();
            
            void print();
            
            void validate();
            
            void write(const std::string& input);
            
            void writeHistory();
            
            void reset();
            
            static void write_history(const std::string& path, const std::vector<avg_point>& history);
            
            [[nodiscard]] inline size_t steps() const
            {
                return count;
            }
            
            [[nodiscard]] std::vector<avg_point> getBestHistory() const
            {
                return best_history;
            }
            
            [[nodiscard]] std::vector<avg_point> getAvgHistory() const
            {
                return avg_history;
            }
            
            [[nodiscard]] individual_point getBestDistance() const
            {
                individual_point p{};
                p.distance = std::numeric_limits<distance_t>::max();
                for (const auto& v : generation_data)
                    for (const auto& i : v.indv)
                        if (i.distance < p.distance)
                            p = i;
                return p;
            }
            
            [[nodiscard]] individual_point getBestCars() const
            {
                individual_point p{};
                p.routes = std::numeric_limits<size_t>::max();
                for (const auto& v : generation_data)
                    for (const auto& i : v.indv)
                        if (i.routes < p.routes || (i.routes == p.routes && i.distance < p.distance))
                            p = i;
                return p;
            }
            
            [[nodiscard]] individual_point getBestFitness() const
            {
                individual_point p{};
                p.fitness = std::numeric_limits<fitness_t>::max();
                for (const auto& v : generation_data)
                    for (const auto& i : v.indv)
                        if (i.fitness < p.fitness)
                            p = i;
                return p;
            }
        
        private:
            size_t count = 0;
            std::int32_t capacity;
            std::vector<record> records;
            std::vector<generation_point> generation_data;
            std::vector<avg_point> best_history;
            std::vector<avg_point> avg_history;
            population current_population;
            random_engine engine;
        public:
            const std::int32_t POPULATION_SIZE;
            const std::int32_t GENERATION_COUNT;
            const std::int32_t TOURNAMENT_SIZE;
            const std::int32_t ELITE_COUNT;
            const double CROSSOVER_RATE;
            const double MUTATION_RATE;
            const double MUTATION2_RATE;
            bool using_fitness = false;
    };
    
    template<typename T>
    inline void print(const T& t)
    {
        for (const auto& v : t)
        {
            BLT_INFO_STREAM << v << " ";
        }
        BLT_INFO_STREAM << "\n";
    }
}

#endif //INC_2006_VRPTW_PARETO_PROGRAM_H
