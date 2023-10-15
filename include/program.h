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

namespace ga
{
    static constexpr std::int32_t CUSTOMER_COUNT = 100;
    static constexpr std::int32_t DEFAULT_POPULATION_SIZE = 500;
    static constexpr std::int32_t DEFAULT_GENERATION_COUNT = 350;
    static constexpr std::int32_t DEFAULT_TOURNAMENT_SIZE = 4;
    static constexpr std::int32_t DEFAULT_ELITE_COUNT = 1;
    static constexpr double DEFAULT_CROSSOVER_RATE = 0.8;
    static constexpr double DEFAULT_MUTATION_RATE = 0.1;
    static constexpr double DEFAULT_MUTATION_2_RATE = 0.01;
    
    // weighted sum fitness
    static constexpr double ALPHA = 100;
    static constexpr double BETA = 0.001;
    
    struct chromosome
    {
        std::array<std::int32_t, CUSTOMER_COUNT> genes{};
    };
    
    struct route
    {
        std::vector<std::int32_t> customers;
        double total_distance = 0;
    };
    
    struct individual
    {
        chromosome c;
        std::vector<route> routes{};
        double total_routes_distance = 0;
        std::int32_t rank = 0;
    };
    
    struct population
    {
        std::vector<individual> pops;
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
            double distance(std::int32_t c1, std::int32_t c2);
            
            double calculate_distance(const route& r);
            
            double validate_route(const route& r);
            
            static bool dominates(const individual& u, const individual& v);
            
            bool is_non_dominated(size_t v);
            
            double weighted_sum_fitness(size_t v);
            
            size_t select_pop(size_t tournament_size);
            
            static void remove_from(const route& r, individual& c);
            
            void insert_to(const route& r_in, individual& c_in);
            
            void reconstruct_populations();
            
            static void reconstruct_chromosome(individual& i);
        
        protected:
            std::vector<route> constructRoute(const chromosome& c);
            
            chromosome createRandomChromosome();
            
            void rankPopulation();
            
            void keepElites(population& pop, size_t n);
            
            void applyCrossover(population& pop);
            
            void applyMutation(population& pop);
            
            void applySecondaryMutation(population& pop);
        
        public:
            program(std::int32_t c, std::vector<record>&& r, std::int32_t popSize = DEFAULT_POPULATION_SIZE,
                    std::int32_t genCount = DEFAULT_GENERATION_COUNT, std::int32_t tourSize = DEFAULT_TOURNAMENT_SIZE,
                    std::int32_t eliteCount = DEFAULT_ELITE_COUNT, double crossoverRate = DEFAULT_CROSSOVER_RATE,
                    double mutationRate = DEFAULT_MUTATION_RATE, double mutation2Rate = DEFAULT_MUTATION_2_RATE):
                    POPULATION_SIZE(popSize), GENERATION_COUNT(genCount), TOURNAMENT_SIZE(tourSize), ELITE_COUNT(eliteCount),
                    CROSSOVER_RATE(crossoverRate), MUTATION_RATE(mutationRate), MUTATION2_RATE(mutationRate)
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
            
            [[nodiscard]] inline size_t steps() const
            {
                return count;
            }
        
        private:
            size_t count = 0;
            std::int32_t capacity;
            std::vector<record> records;
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
